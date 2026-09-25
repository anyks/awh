/**
 * @file backtrack.hpp
 * @date 2026-07-31
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
 * @brief Заголовочный файл исполнения регулярных выражений с возвратом — класс Backtrack,
 *        исполняющий программу единственным состоянием с сохранением точек возврата,
 *        что позволяет сопоставлять конструкции вне регулярного подмножества синтаксиса
 *
 * @section backtrack_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Ряд точек возврата повторения одиночного символа хранится единственной
 *          точкой.</b> Повторение одиночного символа шириной в байт даёт точки,
 *          отличающиеся лишь позицией, убывающей на единицу, поэтому ряд хранится
 *          позицией последнего символа вместе с количеством оставшихся, а возврат
 *          разбирает такую точку на месте, не снимая её с набора. Порядок перебора
 *          длин повторения при этом остаётся прежним. Ряд символов разной ширины
 *          такого хранения не допускает: позиция предшествующего символа
 *          из позиции последующего вычитанием единицы не выводится, и в режиме
 *          разбора UTF-8 точки размещаются по-прежнему на каждый символ.
 *          Замером на выражении «.*needle» получено превосходство в полтора раза,
 *          решение закреплено тестом «Regex.EngineRepeatRun».
 *
 *          <b>Ряд повторения проходится поиском его границы, а не сопоставлением
 *          символов по одному.</b> Вне режима разбора UTF-8 повторение любого символа
 *          доходит до ближайшего перевода строки либо до конца текста, а повторение
 *          символов класса - до первого байта, классу не принадлежащего. Обе границы
 *          отыскиваются набором команд процессора над несколькими байтами сразу
 *          либо единственным обращением к таблице принадлежности, тогда как
 *          сопоставление посимвольное разбирает инструкцию на каждом байте.
 *          Замером получено превосходство в двенадцать раз на выражении «.*needle»
 *          и до трети на выражениях с классами, решение закреплено тестом
 *          «Regex.EngineRepeatSweep».
 *
 *          <b>Таблица принадлежности байтов удерживается для последнего встреченного
 *          класса и отменяется при смене программы.</b> Удержание оправдано тем, что
 *          проход ряда обращается к одному классу на каждом символе. Отмена же
 *          обязательна: ключом таблицы служит адрес класса, а набор классов
 *          принадлежит программе, отчего адрес класса освобождённой программы
 *          способен совпасть с адресом класса иной. Отказ от отмены пробовался
 *          и даёт расхождения с эталонной реализацией на трети выражений
 *          с классами.
 *
 *          <b>Не отсекающая проверка окружения исполняется продолжением исполнения
 *          текущего, а не запуском вложенным.</b> Проверка обыкновенная атомарна:
 *          тело её исполняется запуском отдельным, и точки возврата, телом
 *          накопленные, снимаются вместе с ним. Проверка же не отсекающая -
 *          «(?*...)» и «(?<*...)» - требует обратного: отказ последующего
 *          текста обязан продолжаться перебором тела. Такого запуском
 *          вложенным не выразить вовсе, отчего тело исполняется
 *          в наборе точек общем, а завершается инструкцией
 *          восстановления позиции. Перебор длин проверки
 *          ретроспективной держится живым точкою особой -
 *          признаком перебора в поле ряда позиций.
 *          Решение закреплено тестом
 *          «Regex.InterfaceNonAtomicLookarounds».
 *
 *          <b>Ячейки захвата сбрасываются однажды на сопоставление, а не на
 *          всякую попытку.</b> Сброс на попытку выглядит обязательным - захваты
 *          попытки отказавшей в следующей недействительны, - но избыточен:
 *          исполнение, исчерпав точки возврата, откатывает журнал изменений
 *          до отметки своего входа, а у попытки она - журнал пустой, отчего
 *          всякая ячейка возвращается к значению до попытки. Всякий иной выход
 *          без совпадения ставит ошибку и прекращает сопоставление целиком.
 *          Сброс на попытку стоил внешнего вызова «vector::assign» - 2.2
 *          наносекунды из 24.5 цены попытки. Вернуть его надлежит лишь вместе
 *          с выходом без совпадения, журнала не откатывающим. Решение закреплено
 *          тестом «Regex.EngineStaleCaptures».
 *
 *          <b>Инструкция одиночная ведётся путём отдельным от прохода ряда, и
 *          разбор кода операции в них повторён.</b> Свести оба разбора в один
 *          напрашивается, но возвращает цикл по копиям ряда на путь самый частый:
 *          цикл, заводимый и ради копии единственной, стоил на всякой инструкции.
 *          Путь одиночки ведётся прежним в точности, каким был до прохода рядов,
 *          а проверка пометки цепочки на всяком переходе отмечена маловероятной.
 *          Обе правки судились вместе, сборками выпуска на трёх машинах, по двадцати
 *          одной строке замера: на Эльбрусе выросли двадцать, от 1.6 до 7.8 процента,
 *          на ARM64 - девятнадцать, на x86-64 - пятнадцать, и нигде ни одна
 *          не просела, кроме двух строк x86-64 - «digits-long» и «literal-short».
 *          Обе принадлежат размещению функций: при выравнивании их по 64 байтам
 *          в обеих сборках просадки пропадают, а прибавка остаётся. Расхождение
 *          двух разборов ловит тест
 *          «Regex.EngineSingleAndSeries»: всякий код операции, ряд образующий,
 *          сличается одиночкой и рядом с исполнением без возврата.
 *
 *          <b>Литерал, байтами дословно сличаемый, проходится одним заходом,
 *          а пометка его проверяется без кода операции всюду, кроме Clang
 *          на x86-64.</b> Литерал выражения
 *          компилируется инструкциями одиночного символа по одной на символ,
 *          и проход его стоил захода в разбор кода операции на каждом символе.
 *          Сборка помечает всякую инструкцию литерала длиной остатка и байтами
 *          его - при размещении символа, а не проходом по программе, - а
 *          исполнение сличает байты разом. Литерал, совпавший до конца
 *          текста, поглощается приставкой, а обрыв на байте несовпавшем есть
 *          отказ сразу: учтённых шагов столько же, сколько учёл бы проход
 *          по одной, и исход при всяком пределе шагов и памяти тот же. Шаг
 *          проверки памяти литерал перешагивать вправе: проверка ждёт рубежа -
 *          смотрите решение следующее, - а памяти литерал не размещает вовсе.
 *
 *          Три устройства этого пути выбраны замером, а не вкусом. Пометка
 *          читается в начале разбора символьных кодов без сверки кода
 *          операции: сверка сворачивалась собирателем в самый разбор, таблица
 *          переходов вбирала коды символьные, и класс символов шёл косвенным
 *          переходом. В паре замеров одного прогона на ARM64 строки без
 *          литерала вовсе - «region-nested-heavy» и «recurse-heavy» - теряли
 *          со сверкой 5.3 и 5.9 процента, без неё 1.9 и 2.2. Безопасна
 *          проверка без кода оттого, что
 *          пометка у прочих символьных кодов нулевая по построению - операнды
 *          размещаемой инструкции обнулены конструктором, а зазор их пишет
 *          лишь пометка литерала, - и поверка записи иной не принимает. Метод сличения подстановке запрещён:
 *          подставленный, на x86-64 он отнимал у восьми строк от 1.6 до 6.9
 *          процента, отдельный - ни у одной. Обрыв
 *          отказывает сразу, не исполняя инструкции несовпавшего байта:
 *          выбор ветвей литералов, «alpha|bravo|...», иначе оплачивал сличение
 *          дважды и терял от пяти до семнадцати процентов, а так выигрывает
 *          шестнадцать.
 *
 *          У Clang на x86-64 разбор устроен иначе, признаком
 *          «AWH_REGEX_LITERAL_CASE»: у одиночного символа свой случай, таблица
 *          переходов растянута на все коды, и пометку читает лишь он. Там это
 *          устройство выигрывает: против пути общего оно подняло на FreeBSD
 *          (clang 19, десять кругов вперемежку) двенадцать строк, до 8.6
 *          процента, и опустило три, «alpha|bravo|...» - на 6.7; в сборке
 *          с выравниванием всех блоков - десять и две. У GCC на x86-64 (NetBSD,
 *          GCC 10.5) свой случай проигрывает, как и на ARM64: в сборке
 *          с выравниванием всех переходов он опустил восемнадцать строк, до 12.9
 *          процента, и GCC оставлен на пути общем. Код ARM64, Эльбруса и GCC
 *          от признака не изменился вовсе: сборки их совпали с прежними до байта.
 *
 *          Итог мерился щупом «rowtime» против основы, двенадцатью кругами
 *          вперемежку. На ARM64 выросли тринадцать строк, от 3.3 до 23.1
 *          процента: «(?:HT|TP)/1» - 23.1, «GET|POST|PUT|DELETE|HEAD|OPTIONS» -
 *          19.1, «(?m)^(GET|POST) (\S+) HTTP/...» - 18.2. Строки прочие ARM64
 *          качает размещением кода: две правки, смысла не менявшие, - иная
 *          запись рубежа проверки памяти и снятый довод метода сличения, -
 *          сдвигали «\((?:[^()]|(?R))*\)» с минус 2.8 до минус 7.6 процента.
 *          Цену самого пути показывает
 *          сборка, где всякий блок, куда ведёт переход, выровнен по 64 байтам:
 *          там просели семь строк, от 1.1 до 3.7 процента, и все они богаты
 *          шагами класса символов - пометку читает и такой шаг. Три из них
 *          против эталона уже на единице либо ниже и опускаются ещё:
 *          «[0-9]{3,5}» - с 0.98 до 0.95, «(\w+)@(\w+)\.(\w+)» - с 0.91 до 0.89,
 *          «(\w+) \1» - с 1.00 до 0.98. Строк ниже единицы при этом становится
 *          девять взамен десяти, а слабейшая, «(?m)^(GET|POST) ...», поднимается
 *          с 0.75 до 0.89. На Эльбрусе выросли шестнадцать строк, от 2.0 до 18.7
 *          процента, и просели три, до 3.3. На x86-64 у Clang со своим случаем
 *          выросли одиннадцать, до 15.8, и просели восемь, до 4.4; у GCC путём
 *          общим - тринадцать, до 38.2, и семь, до 6.3.
 *
 *          Сборке выражения путь заметного не стоит: пометка ставится при
 *          размещении символа, и одной лишь прямой программе - развёрнутую
 *          исполняет детерминированное исполнение, пометки не читающее.
 *          Проходом по программе, каким пометка ставилась прежде, сборка
 *          теряла 4.1 процента: шёл он по всякой инструкции, и дважды, а
 *          поправки дешёвые - иная запись циклов, слияние с проходом рядов -
 *          этого не снимали. Пометка при размещении со сборкой, пометки
 *          не ставящей вовсе, вровень: минус 0.2 процента тридцатью кругами
 *          вперемежку. Числа и условия замеров -
 *          в «benchmark/regex/COMPARISON.md». Закреплено проверками
 *          «Regex.EngineLiteralRun», «Regex.EngineLiteralPlacement»
 *          и «Regex.StorageForgedLiteral»; путь, какой машине не достался,
 *          проверяется сборкой с признаком переопределённым.
 *
 *          <b>Проверка допустимого объёма памяти ждёт рубежа, а не шага,
 *          кратного двумстам пятидесяти шести.</b> Счётчик шагов прибавляется
 *          и пачкой: проход ряда, цепочки и копий ряда одинаковых инструкций,
 *          продвижение ленивого ряда, отмена захватов и литерал учитывают
 *          разом всё пройденное, и шаг кратный перешагивался - предел памяти
 *          не проверялся, пока пачка не приходилась на кратный шаг случаем.
 *          Щупом установлено: «^(?:x|a{8})*$» на сорока трёх обходах при
 *          пределе в два килобайта находил совпадение там, где проход по одной
 *          исчерпывал память, - 740 расхождений из 7 200 сочетаний, и ни одного
 *          с рубежом. Рубеж есть ближайший шаг, кратный двумстам пятидесяти
 *          шести, впереди счётчика; проверка ведётся на всяком шаге за ним
 *          и переносит его дальше. Сличений на шаг остаётся два, как и было,
 *          и от рубежа единого, в цикле отброшенного, этот отличен тем, что
 *          пределу шагов не служит. Цены своей рубеж не несёт - перенос его
 *          лежит на пути редком, - и замер обычных сборок ARM64 показал лишь
 *          размещение кода: против исполнения без рубежа строки сдвигались
 *          от минус 4.3 до плюс 12 процентов. Закреплено проверкой
 *          «Regex.EngineMemoryCheckpoint».
 *
 * \~english
 * @brief Header file of the execution of regular expressions with backtracking — the Backtrack class,
 *        which executes the program by a single state while saving backtracking points,
 *        which allows matching constructs outside the regular subset of the syntax
 * @section backtrack_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>A run of backtracking points of a repetition of a single character is kept as a single
 *          point.</b> A repetition of a single character one byte wide yields points
 *          that differ only by the position, decreasing by one, therefore the run is kept
 *          as the position of the last character together with the number of the remaining ones, and backtracking
 *          takes such a point apart in place without removing it from the set. The order of enumerating
 *          the repetition lengths remains the former one. A run of characters of different widths
 *          admits no such keeping: the position of the preceding character
 *          is not derived from the position of the following one by subtracting one, and in the UTF-8
 *          parsing mode the points are placed as before, one per character.
 *          Measurement on the «.*needle» expression yielded a one-and-a-half-fold advantage,
 *          the decision is fixed by the «Regex.EngineRepeatRun» test.
 *          <b>A run of a repetition is walked by searching for its boundary rather than by matching
 *          the characters one by one.</b> Outside the UTF-8 parsing mode a repetition of any character
 *          reaches the nearest line feed or the end of the text, and a repetition
 *          of the characters of a class the first byte not belonging to the class. Both boundaries
 *          are located by processor instructions over several bytes at once
 *          or by a single reference to the belonging table, whereas
 *          character-by-character matching takes the instruction apart at every byte.
 *          Measurement yielded a twelvefold advantage on the «.*needle» expression
 *          and up to a third on the expressions with classes, the decision is fixed by the
 *          «Regex.EngineRepeatSweep» test.
 *          <b>The byte belonging table is held for the last encountered
 *          class and is cancelled when the program changes.</b> The holding is justified by the fact that
 *          walking a run refers to one class at every character. The cancellation, on the other hand,
 *          is mandatory: the key of the table is the address of the class, and the set of classes
 *          belongs to the program, which is why the address of a class of a released program
 *          is able to coincide with the address of a class of another one. Giving up the cancellation was tried
 *          and yields divergences from the reference implementation on a third of the expressions
 *          with classes.
 *
 *          <b>A non-atomic lookaround check is executed as a continuation of the current
 *          run rather than as a nested run.</b> An ordinary check is atomic:
 *          its body is executed as a separate run, and the backtracking points
 *          accumulated by the body are removed together with it. A non-atomic check —
 *          «(?*...)» and «(?<*...)» — requires the opposite: a failure of the following
 *          text must continue with a walk over the body. That cannot be expressed
 *          by a nested run at all, whereby the body is executed
 *          in the common set of points and ends with an instruction
 *          restoring the position. The walk over the lengths of a lookbehind
 *          check is kept alive by a special point — by a flag
 *          of the walk in the field of the run of positions.
 *          The decision is pinned by the test
 *          «Regex.InterfaceNonAtomicLookarounds».
 *
 *          <b>The capture cells are reset once per matching rather than at every
 *          attempt.</b> A reset per attempt looks mandatory - the captures of a failed
 *          attempt are invalid in the next one, - but is redundant: the execution,
 *          having exhausted the backtracking points, rolls the change log back to the
 *          mark of its entry, and for an attempt that mark is an empty log, whereby
 *          every cell returns to its value before the attempt. Every other exit
 *          without a match sets an error and stops the matching as a whole.
 *          The reset per attempt cost an external call of «vector::assign» - 2.2
 *          nanoseconds out of the 24.5 of the price of an attempt. It is to be brought
 *          back only together with an exit without a match that does not roll the log
 *          back. The decision is pinned by the «Regex.EngineStaleCaptures» test.
 *
 *          <b>A single instruction is executed by a path separate from walking
 *          a row, and the dispatch on the operation code is repeated in them.</b>
 *          Merging both dispatches into one suggests itself, but brings the loop over
 *          the copies of a row back onto the most frequent path: the loop, set up even
 *          for a sole copy, cost at every instruction. The path of a single instruction
 *          is kept exactly the former one it was before walking rows, and the check
 *          of the chain mark on every split is marked as unlikely. Both edits were
 *          judged together, by release builds on three machines, over twenty one
 *          rows of the measurement: on Elbrus twenty grew, from 1.6 to 7.8 percent,
 *          on ARM64 nineteen, on x86-64 fifteen, and nowhere did any drop, except
 *          two rows of x86-64 - «digits-long» and «literal-short». Both belong to
 *          the placement of functions: with them aligned by 64 bytes in both builds
 *          the drops disappear, and the gain remains. A divergence of the two
 *          dispatches is caught by the
 *          «Regex.EngineSingleAndSeries» test: every operation code forming a row
 *          is compared, as a single and as a row, with the execution without
 *          backtracking.
 *
 *          <b>A literal compared by bytes verbatim is walked in one trip, and its
 *          mark is checked without the operation code everywhere except Clang
 *          on x86-64.</b> A literal of the
 *          expression is compiled into single character instructions, one per
 *          character, and walking it cost a trip through the dispatch of the
 *          operation code on every character. The build marks every instruction of
 *          the literal with the length of the remainder and its bytes — when a
 *          character is placed rather than by a pass over the program, — and the
 *          execution compares the bytes at once. A literal that coincided up to the
 *          end of the text is consumed by the prefix, while a break on a non-matching
 *          byte is an immediate refusal: as many steps are counted as walking one by
 *          one would count, and the outcome is the same at every limit of steps and
 *          memory. The literal is entitled to step over the step of the memory check:
 *          the check waits for a checkpoint — see the next decision, — and the literal
 *          allocates no memory at all.
 *
 *          Three features of this path are chosen by measurement, not by taste. The
 *          mark is read at the beginning of the dispatch of character codes without
 *          checking the operation code: the check was folded by the compiler into the
 *          dispatch itself, the jump table absorbed the character codes, and the
 *          character class went through an indirect jump. In a pair of measurements of
 *          one run on ARM64 the rows without any literal — «region-nested-heavy» and
 *          «recurse-heavy» — lost 5.3 and 5.9 per cent with the check, 1.9 and 2.2
 *          without it. The check without the code is safe because the mark of the
 *          other character codes is zero by construction — the operands of a placed
 *          instruction are zeroed by the constructor, and only the mark of a literal
 *          writes their gap, — and the verification of the record accepts no other. The comparison method is forbidden to be inlined:
 *          inlined, on x86-64 it took from 1.6 to 6.9 per cent from eight rows,
 *          separate — from none. A break refuses at once, without executing the
 *          instruction of the non-matching byte: an alternation of literals,
 *          «alpha|bravo|...», otherwise paid for the comparison twice and lost from
 *          five to seventeen per cent, and so it gains sixteen.
 *
 *          With Clang on x86-64 the dispatch is arranged differently, by the
 *          «AWH_REGEX_LITERAL_CASE» flag: a single character has its own case, the
 *          jump table is stretched over all the codes, and only that case reads the
 *          mark. There this arrangement wins: against the common path it raised twelve
 *          rows on FreeBSD (clang 19, ten interleaved rounds), by up to 8.6 per cent,
 *          and lowered three, «alpha|bravo|...» by 6.7; in the build with all blocks
 *          aligned — ten and two. With GCC on x86-64 (NetBSD, GCC 10.5) the own case
 *          loses, as on ARM64: in the build with all jump targets aligned it lowered
 *          eighteen rows, by up to 12.9 per cent, and GCC is left on the common path.
 *          The code of ARM64, Elbrus and GCC did not change from the flag at all: their
 *          builds coincided with the former ones byte for byte.
 *
 *          The result was measured by the «rowtime» probe against the base, with
 *          twelve interleaved rounds. On ARM64 thirteen rows grew, from 3.3 to 23.1
 *          per cent: «(?:HT|TP)/1» — 23.1, «GET|POST|PUT|DELETE|HEAD|OPTIONS» — 19.1,
 *          «(?m)^(GET|POST) (\S+) HTTP/...» — 18.2. The other rows are swung on ARM64
 *          by the placement of code: two edits that changed no meaning — another
 *          writing of the checkpoint of the memory check and the removed argument of
 *          the comparison method — moved «\((?:[^()]|(?R))*\)» from minus 2.8 to
 *          minus 7.6 per cent. The price of the path itself is shown by
 *          a build where every block a jump leads to is aligned by 64 bytes: there
 *          seven rows dropped, from 1.1 to 3.7 per cent, and all of them are rich in
 *          steps of a character class — such a step reads the mark too. Three of them
 *          are already at one or below against the reference and go lower still:
 *          «[0-9]{3,5}» — from 0.98 to 0.95, «(\w+)@(\w+)\.(\w+)» — from 0.91 to 0.89,
 *          «(\w+) \1» — from 1.00 to 0.98. The rows below one become nine instead of
 *          ten, and the weakest, «(?m)^(GET|POST) ...», rises from 0.75 to 0.89. On
 *          Elbrus sixteen rows grew, from 2.0 to 18.7 per cent, and three dropped, by up
 *          to 3.3. On x86-64 with Clang and the own case eleven grew, by up to 15.8, and
 *          eight dropped, by up to 4.4; with GCC on the common path — thirteen, by up to
 *          38.2, and seven, by up to 6.3.
 *
 *          The path costs the building of an expression nothing noticeable: the mark
 *          is set when a character is placed, and to the forward program alone — the
 *          reverse one is executed by the deterministic execution, which does not read
 *          the mark. With the pass over the program that set the mark before, the
 *          building lost 4.1 per cent: it went over every instruction, and twice, and
 *          cheap remedies — another writing of the loops, a merge with the pass of
 *          series — did not remove that. The mark at placement is on a par with a
 *          building that sets no mark at all: minus 0.2 per cent over thirty
 *          interleaved rounds. The numbers and the conditions of the
 *          measurements are in «benchmark/regex/COMPARISON.md». Pinned by the
 *          «Regex.EngineLiteralRun», «Regex.EngineLiteralPlacement» and
 *          «Regex.StorageForgedLiteral» tests; the path a machine did not get is
 *          checked by a build with the flag overridden.
 *
 *          <b>The check of the admissible amount of memory waits for a checkpoint
 *          rather than for a step that is a multiple of two hundred fifty-six.</b>
 *          The step counter is also increased in bundles: walking a row, a chain and
 *          the copies of a row of identical instructions, advancing a lazy row,
 *          undoing captures and a literal count everything passed at once, and the
 *          multiple step was stepped over — the memory limit was not checked until a
 *          bundle landed on a multiple step by chance. A probe established:
 *          «^(?:x|a{8})*$» on forty-three passes with a limit of two kilobytes found a
 *          match where walking one by one exhausted the memory — 740 divergences out
 *          of 7 200 combinations, and none with the checkpoint. The checkpoint is the
 *          nearest step that is a multiple of two hundred fifty-six ahead of the
 *          counter; the check is made at every step past it and moves it further.
 *          Two comparisons per step remain, as before, and this checkpoint differs
 *          from the single threshold rejected in the loop in that it does not serve
 *          the step limit. The checkpoint carries no price of its own — moving it lies
 *          on a rare path, — and the measurement of ordinary ARM64 builds showed only
 *          the placement of code: against the execution without the checkpoint the
 *          rows moved from minus 4.3 to plus 12 per cent. Pinned by the
 *          «Regex.EngineMemoryCheckpoint» test.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <utility>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "pike.hpp"
#include "text.hpp"
#include "program.hpp"

/**
 * \~russian
 * @brief Признак разбора литерала своим случаем кода операции
 *
 * @details Голову литерала исполнение с возвратом узнаёт пометкой, и место
 *          проверки её в разборе кода операции выбрано замером по архитектуре
 *          и собирателю. У Clang на x86-64 у одиночного символа свой случай:
 *          таблица переходов растянута на все коды, класс символов идёт ею
 *          без сравнения диапазона, а пометку читает лишь одиночный символ.
 *          Прочие сборки проверяют пометку в начале разбора кодов символьных
 *          общего без кода операции, куда коды символьные уходят сравнением
 *          диапазона: свой случай стоил там до тринадцати процентов - на ARM64
 *          строкам со скобками, на x86-64 у GCC восемнадцати строкам в сборке,
 *          где всякий блок, куда ведёт переход, выровнен.
 *
 *          Признак заведён в заголовке, а не в исполнении, затем, что его читают
 *          и проверки: пометка у класса символов опасна лишь на пути без своего
 *          случая. Сборка переопределяет его, «-DAWH_REGEX_LITERAL_CASE=0» либо
 *          «=1»: так проверяется на всякой машине и путь, какой ей не достался,
 *          и так же заводится условие под собиратель, какой иное устройство
 *          предпочтёт. Сборка ARM64EC у Visual Studio ставит признак x86-64 тоже,
 *          а код её машинный - ARM64, отчего она исключена.
 *
 * \~english
 * @brief Flag of dispatching a literal by its own opcode case
 *
 * @details Execution with backtracking recognises the head of a literal by its mark,
 *          and the place where the mark is checked in the opcode dispatch was chosen
 *          by measurement per architecture and compiler. With Clang on x86-64 a single
 *          character has its own case: the jump table is stretched over all the codes,
 *          the character class goes through it without a range comparison, and only
 *          a single character reads the mark. The other builds check the mark at the
 *          start of the common dispatch of character opcodes without the opcode, where
 *          the character opcodes arrive by a range comparison: an own case cost up to
 *          thirteen per cent there — on ARM64 to the rows with brackets, on x86-64 with
 *          GCC to eighteen rows in a build where every block a jump leads to is aligned.
 *
 *          The flag lives in the header rather than in the implementation because the
 *          tests read it as well: a mark on a character class is dangerous only on the
 *          path without an own case. The build overrides it, «-DAWH_REGEX_LITERAL_CASE=0»
 *          or «=1»: this way every machine checks the path it did not get as well, and
 *          a condition for a compiler preferring the other arrangement is added the same
 *          way. The ARM64EC build of Visual Studio sets the x86-64 flag too, while its
 *          machine code is ARM64, therefore it is excluded.
 *
 * \~
 */
#if !defined(AWH_REGEX_LITERAL_CASE)
	/**
	 * Если сборка ведётся собирателем Clang под x86-64
	 */
	#if defined(__clang__) && (defined(__x86_64__) || (defined(_M_X64) && !defined(_M_ARM64EC)))
		/**
		 * Литерал разбирается своим случаем кода операции
		 */
		#define AWH_REGEX_LITERAL_CASE 1
	/**
	 * Если сборка ведётся под прочие архитектуры либо прочими собирателями
	 */
	#else
		/**
		 * Литерал разбирается пометкой без кода операции
		 */
		#define AWH_REGEX_LITERAL_CASE 0
	#endif
#endif

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
		 * @brief Наибольшее допустимое количество шагов сопоставления
		 *
		 * @details Исполнение с возвратом требует времени, растущего с длиной текста
		 *          показательно на выражениях с вложенными кванторами, поэтому объём
		 *          работы ограничивается. Исчерпание предела прекращает сопоставление
		 *          с ошибкой, а не молчаливым отказом от совпадения.
		 *
		 * \~english
		 * @brief Largest admissible number of matching steps
		 * @details Execution with backtracking requires time growing with the length of the text
		 *          exponentially on expressions with nested quantifiers, therefore the amount
		 *          of work is bounded. Exhausting the limit stops the matching
		 *          with an error rather than with a silent refusal of a match.
		 *
		 * \~
		 */
		constexpr size_t MAX_STEPS = 0x989680;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество точек возврата
		 *
		 * @details Предел количества шагов ограничивает время сопоставления, но не
		 *          занимаемую им память: каждый переход по двум ветвям размещает точку
		 *          возврата, поэтому их количество ограничивается отдельно.
		 *
		 * \~english
		 * @brief Largest admissible number of backtracking points
		 * @details The limit on the number of steps bounds the time of the matching, but not
		 *          the memory it occupies: every two-branch jump places a backtracking
		 *          point, therefore their number is bounded separately.
		 *
		 * \~
		 */
		constexpr size_t MAX_POINTS = 0x100000;

		/**
		 * \~russian
		 * @brief Признак цепочки ограниченного повторения, одним ходом не пройденной
		 *
		 * @details Проход цепочки берётся не у всякого тела, и отказ его надлежит
		 *          отличить от прохода, поглотившего копий нуль: первый велит пройти
		 *          цепочку по инструкциям, второй же означает переход к выходу
		 *          немедленный. Признаком служит значение, количеством копий
		 *          недостижимое: копий в цепочке не более разрядности поля пометки.
		 *
		 * \~english
		 * @brief Mark of a chain of a bounded repetition not walked in one move
		 * @details The walk of a chain is not taken for every body, and its refusal must be
		 *          told apart from a walk that consumed zero copies: the first orders to walk
		 *          the chain by instructions, whereas the second means an immediate transition
		 *          to the exit. The mark is a value unreachable by a number of copies: there
		 *          are no more copies in a chain than the width of the field of the mark.
		 *
		 * \~
		 */
		constexpr size_t UNCHAINED = static_cast <size_t> (-1);

		/**
		 * \~russian
		 * @brief Признак обрыва сличения литерала на байте, литералу не отвечающем
		 *
		 * @details Сличение литерала отдаёт количество совпавших символов, а обрыв
		 *          надлежит отличить от литерала, совпавшего до конца текста: первый
		 *          есть отказ инструкции следующей, второй велит исполнению
		 *          продолжать. Признак прибавляется
		 *          к количеству и количеством недостижим: литерал пометки
		 *          не длиннее MAX_LITERAL.
		 *
		 * \~english
		 * @brief Mark of a break of comparing a literal on a byte not matching the literal
		 * @details Comparing a literal returns the number of the coinciding characters, and a
		 *          break must be told apart from a literal that coincided up to the end of the
		 *          text: the first is a refusal of the next instruction, the second orders the
		 *          execution to continue. The mark is added to the number
		 *          and is unreachable by it: the literal of a mark is not longer than MAX_LITERAL.
		 *
		 * \~
		 */
		constexpr size_t BROKEN = 0x100;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество кадров рекурсивных вызовов
		 *
		 * @details Кадр вызова сохраняется до отмены вызова возвратом, поэтому их
		 *          количество растёт с числом выполненных вызовов, а не с глубиной
		 *          вложенности, и ограничивается отдельно от неё.
		 *
		 * \~english
		 * @brief Largest admissible number of recursive call frames
		 * @details A call frame is kept until the call is cancelled by a return, therefore their
		 *          number grows with the number of performed calls rather than with the nesting
		 *          depth, and is bounded separately from it.
		 *
		 * \~
		 */
		constexpr size_t MAX_FRAMES = 0x100000;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество записей журнала изменений
		 *
		 * @details Возврат из рекурсивного вызова отменяет выполненные им захваты
		 *          повторной записью прежних значений, что дописывает в журнал его
		 *          собственный хвост. Вложенные вызовы наращивают журнал показательно,
		 *          поэтому его размер ограничивается отдельно.
		 *
		 * \~english
		 * @brief Largest admissible number of change log records
		 * @details A return from a recursive call cancels the captures it performed
		 *          by writing the former values again, which appends its own tail to the
		 *          log. Nested calls grow the log exponentially,
		 *          therefore its size is bounded separately.
		 *
		 * \~
		 */
		constexpr size_t MAX_JOURNAL = 0x400000;

		/**
		 * \~russian
		 * @brief Наибольшая допустимая глубина рекурсивных вызовов
		 *
		 * @details Рекурсивный вызов подвыражения способен не продвигаться по тексту,
		 *          поэтому глубина вызовов ограничивается независимо от объёма работы.
		 *
		 * \~english
		 * @brief Largest admissible depth of recursive calls
		 * @details A recursive call of a subexpression is able not to advance through the text,
		 *          therefore the depth of the calls is bounded independently of the amount of work.
		 *
		 * \~
		 */
		constexpr size_t MAX_RECURSION = 1000;
		/**
		 * \~russian
		 * @brief Наибольшая допустимая вложенность исполнений программы
		 *
		 * @details Тело проверки окружения исполняется вложенным исполнением,
		 *          и вложенность его ограничена стеком машины, а не памятью
		 *          сопоставления. Программа, тело проверки какой указывает
		 *          на неё же, дала бы вложенность бесконечную и исчерпание
		 *          стека: собранная программа такого не содержит, а поддельная
		 *          запись хранилища - вполне.
		 *
		 * \~english
		 * @brief Largest admissible nesting of executions of the program
		 * @details The body of a lookaround assertion is executed by a nested
		 *          execution, and its nesting is bounded by the stack of the machine
		 *          rather than by the memory of the matching. A program whose body
		 *          of an assertion points at itself would give an infinite nesting
		 *          and an exhaustion of the stack: a built program contains no such
		 *          thing, whereas a forged record of the storage may well.
		 *
		 * \~
		 */
		constexpr size_t MAX_NESTED = 256;

		/**
		 * \~russian
		 * @brief Наибольшее количество таблиц принадлежности байтов классам
		 *
		 * @details Таблица заводится на каждый класс, пройденный повторением, и живёт
		 *          до смены программы. Программа надстройки Grok несёт классов десятки
		 *          тысяч, и без предела таблицы её заняли бы мегабайты на каждый
		 *          объект исполнения. По достижении предела таблицы сбрасываются
		 *          разом, как сбрасывается кэш состояний детерминированного исполнения:
		 *          предел держит память, а выражение обычное его не достигает вовсе.
		 *
		 * \~english
		 * @brief Largest number of the byte belonging tables of the classes
		 * @details A table is set up for every class walked by a repetition, and lives
		 *          until the program changes. A program of the Grok extension carries tens
		 *          of thousands of classes, and without a limit its tables would take
		 *          megabytes for every execution object. Upon reaching the limit the tables
		 *          are reset all at once, as the state cache of the deterministic execution is:
		 *          the limit bounds the memory, while an ordinary expression never reaches it.
		 *
		 * \~
		 */
		constexpr size_t MAX_TABLES = 0x400;

		/**
		 * \~russian
		 * @brief Класс исполнения регулярного выражения с возвратом
		 *
		 * @details Класс исполняет программу единственным состоянием, сохраняя точки
		 *          возврата при переходе по двум ветвям и возвращаясь к ним при отказе
		 *          сопоставления. В отличие от исполнения без возврата, способ допускает
		 *          конструкции, требующие обращения к ранее захваченному тексту, но
		 *          требует времени, растущего с длиной текста показательно.
		 *
		 * \~english
		 * @brief Class of the execution of a regular expression with backtracking
		 * @details The class executes the program by a single state, saving backtracking
		 *          points at a two-branch jump and returning to them on a matching
		 *          failure. Unlike execution without backtracking, the way admits
		 *          constructs requiring a reference to previously captured text, but
		 *          requires time growing with the length of the text exponentially.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Backtrack {
			private:
				/**
				 * \~russian
				 * @brief Точка возврата исполнения программы
				 *
				 * \~english
				 * @brief Backtracking point of the execution of the program
				 *
				 * \~
				 */
				typedef struct Point {
					/**
					 * \~russian
					 * Флаг восстановления кадра рекурсивного вызова
					 *
					 * @details Точка возврата с установленным флагом не продолжает
					 *          исполнения, а восстанавливает исполняемый рекурсивный
					 *          вызов, после чего возврат продолжается далее.
					 *
					 * \~english
					 * Flag of restoring a recursive call frame
					 * @details A backtracking point with the flag set does not continue
					 *          the execution but restores the executed recursive
					 *          call, after which the backtracking continues further.
					 *
					 * \~
					 */
					bool frame;
					// Адрес инструкции, с которой продолжается исполнение
					address_t pc;
					// Позиция в тексте, с которой продолжается исполнение
					size_t pos;
					// Размер журнала изменений ячеек захвата на момент сохранения
					size_t journal;
					// Размер журнала изменений отметок атомарных групп на момент сохранения
					size_t remarks;
					/**
					 * \~russian
					 * Количество оставшихся позиций ряда повторения одиночного символа
					 *
					 * @details Повторение одиночного символа шириной в один байт даёт
					 *          ряд точек возврата, отличающихся лишь позицией, убывающей
					 *          на единицу. Такой ряд хранится единственной точкой,
					 *          отчего проход ряда длиной в текст размещает одну точку
					 *          взамен точки на каждый байт текста.
					 *
					 * \~english
					 * Number of the remaining positions of a run of a repetition of a single character
					 * @details A repetition of a single character one byte wide yields
					 *          a run of backtracking points differing only by the position, decreasing
					 *          by one. Such a run is kept as a single point,
					 *          which is why walking a run as long as the text places one point
					 *          instead of a point per every byte of the text.
					 *
					 * \~
					 */
					size_t span;
					/**
					 * \~russian
					 * Вид глагола управления, точку разместившего
					 *
					 * @details Значение выводится увеличенным на единицу, а нуль
					 *          означает точку обыкновенную: глаголы управления
					 *          размещают точку, исполнения не продолжающую, - возврат
					 *          в неё прекращает попытку сопоставления, а вид глагола
					 *          указывает, как её продолжать.
					 *
					 * \~english
					 * Kind of the control verb that placed the point
					 * @details The value is yielded increased by one, whereas zero
					 *          means an ordinary point: the control verbs place a point
					 *          that does not continue the execution — backtracking into it
					 *          terminates the matching attempt, and the kind of the verb
					 *          tells how to continue it.
					 *
					 * \~
					 */
					uint8_t control;
					/**
					 * \~russian
					 * Флаг перебора длин не отсекающей ретроспективной проверки
					 *
					 * @details Проверка ретроспективная сопоставляется отступом назад
					 *          на длину проверяемой последовательности, и длины её
					 *          перебираются от наибольшей. Проверка обыкновенная
					 *          перебирает их запуском вложенным, тогда как проверка
					 *          не отсекающая обязана держать перебор живым и после
					 *          выполнения своего: точка с установленным флагом
					 *          длину очередную и несёт - в поле ряда позиций
					 *
					 * \~english
					 * Flag of the walk over the lengths of a non-atomic lookbehind check
					 * @details A lookbehind check is matched at an offset backwards
					 *          by the length of the checked sequence, and its lengths
					 *          are walked from the largest. An ordinary check
					 *          walks them by a nested run, whereas a non-atomic
					 *          check must keep the walk alive even after
					 *          its own fulfilment: a point with the flag set
					 *          carries the current length — in the field of the run of positions
					 *
					 * \~
					 */
					uint8_t seek;
					// Номер ячейки отметки ветви охватывающей группы глагола перехода
					uint32_t cell;
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
					Point() noexcept : frame(false), pc(0), pos(0), journal(0), span(0), control(0), seek(0), cell(0) {}
				} point_t;
			private:
				/**
				 * \~russian
				 * @brief Запись журнала изменений ячеек захвата
				 *
				 * @details Журнал сохраняет прежние значения изменённых ячеек, благодаря
				 *          чему возврат восстанавливает состояние захвата без хранения
				 *          набора ячеек целиком в каждой точке возврата.
				 *
				 * \~english
				 * @brief Record of the change log of the capture cells
				 * @details The log keeps the former values of the changed cells, thanks to
				 *          which backtracking restores the state of the capture without keeping
				 *          the whole set of cells at every backtracking point.
				 *
				 * \~
				 */
				typedef struct Change {
					// Номер изменённой ячейки захвата
					uint32_t slot;
					// Прежнее значение изменённой ячейки захвата
					size_t value;
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
					Change() noexcept : slot(0), value(0) {}
				} change_t;
			private:
				/**
				 * \~russian
				 * @brief Таблица принадлежности значений байта классу символов
				 *
				 * @details Набор режимов хранится вместе с таблицей: принадлежность
				 *          зависит от режима сопоставления без учёта регистра наравне
				 *          с самим классом.
				 *
				 * \~english
				 * @brief Table of the belonging of the byte values to a character class
				 * @details The set of modes is kept together with the table: the belonging
				 *          depends on the case-insensitive matching mode on a par with
				 *          the class itself.
				 *
				 * \~
				 */
				typedef struct Table {
					// Набор режимов, при каком построена таблица
					uint32_t modes;
					// Принадлежность значений байта классу символов
					uint8_t bytes[0x100];
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
					Table() noexcept : modes(0), bytes{} {}
				} table_t;
				/**
				 * \~russian
				 * @brief Разрешение таблицы принадлежности байтов классу символов
				 *
				 * @details Разрешение ведёт адрес таблицы вместе с набором режимов,
				 *          при каком та построена, и служит прямым путём от номера
				 *          класса к таблице: набор таблиц ведётся номерами, и путь
				 *          через него - цепочка обращений зависимых, задержкой памяти
				 *          ограниченная. Адрес действителен, покуда набор таблиц
				 *          не перемещался: обновляются разрешения целиком и ровно
				 *          там, где таблица заводится.
				 *
				 * \~english
				 * @brief Resolution of the byte belonging table of a character class
				 * @details A resolution carries the address of the table together with the set
				 *          of modes it was built with, and serves as a direct way from the number
				 *          of the class to the table: the set of tables is kept by numbers, and
				 *          the way through it is a chain of dependent references bounded by the
				 *          latency of the memory. The address is valid as long as the set of tables
				 *          has not moved: the resolutions are updated as a whole and exactly where
				 *          a table is set up.
				 *
				 * \~
				 */
				typedef struct Lookup {
					// Набор режимов, при каком построена таблица
					uint32_t modes;
					// Адрес таблицы принадлежности значений байта классу
					const uint8_t * bytes;
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
					Lookup() noexcept : modes(0), bytes(nullptr) {}
				} lookup_t;
			private:
				// Исполняемая программа регулярного выражения
				const program_t * _program;
			private:
				// Текст, по которому выполняется сопоставление
				string_view _text;
			private:
				// Позиция начала текущей попытки сопоставления
				size_t _start;
			private:
				/**
				 * \~russian
				 * Позиция, с которой начата попытка сопоставления нынешняя
				 *
				 * @details Глагол переноса с именем, отметку найдя в положении
				 *          не позднее начала попытки, не правит вовсе: перенос
				 *          назад зациклил бы обход позиций, а прекращение попытки
				 *          отняло бы ветви, глаголу не пройденные.
				 *
				 * \~english
				 * Position from which the current matching attempt was started
				 * @details The moving verb with a name, having found a mark at a position
				 *          no later than the beginning of the attempt, has no effect at all:
				 *          moving backwards would loop the position traversal, while terminating
				 *          the attempt would take away the branches not passed by the verb.
				 *
				 * \~
				 */
				size_t _attempt;

			private:
				// Количество выполненных шагов сопоставления
				size_t _steps;
			private:
				/**
				 * \~russian
				 * Счётчики мер работы сопоставления, разысканием ведомые
				 *
				 * @details Счётчики ведутся полями, а не общим учётом «probe_t»:
				 *          приращение разделяемого счётчика внутри проверки
				 *          принадлежности классу раздуло бы её собственное время
				 *          в образце стека - том самом орудии, каким разыскание
				 *          и ведётся. Поля складываются дёшево, а в общий учёт
				 *          вносятся разом по завершении сопоставления.
				 *
				 *          Поля заведены безусловно, а приращение их ограждено
				 *          признаком сборки «AWH_REGEX_PROBING»: ограда вокруг
				 *          самих полей развела бы состав класса между сборками,
				 *          и единица трансляции, собранная без признака, читала
				 *          бы соседние поля по смещениям чужим.
				 *
				 * \~english
				 * Counters of the measures of the work of matching, kept for the investigation
				 * @details The counters are kept as fields rather than in the shared "probe_t"
				 *          accounting: incrementing a shared counter inside the check of
				 *          membership in a character class would inflate that check's own time
				 *          in the stack sample — the very instrument by which the investigation
				 *          is carried out. Fields add cheaply, and are contributed to the shared
				 *          accounting in one go upon completion of the match.
				 *
				 *          The fields are declared unconditionally while their incrementing is
				 *          guarded by the build flag "AWH_REGEX_PROBING": a guard around the
				 *          fields themselves would diverge the layout of the class between
				 *          builds, and a translation unit built without the flag would read
				 *          the neighbouring fields at foreign offsets.
				 *
				 * \~
				 */
				size_t _saves;
				// Количество выполненных проверок принадлежности байта классу символов
				size_t _checks;
				// Количество размещённых точек возврата
				size_t _points_spent;
				// Количество заведённых кадров вызова подвыражения
				size_t _frames_spent;
				// Количество обходов цикла исполнения с возвратом
				size_t _rounds;
			private:
				/**
				 * \~russian
				 * Допустимое количество шагов сопоставления
				 *
				 * @details Объём действует на одно сопоставление и восстанавливается
				 *          предельным по его завершении.
				 *
				 * \~english
				 * Admissible number of matching steps
				 * @details The amount acts on one match and is restored to
				 *          the limiting one on its completion.
				 *
				 * \~
				 */
				size_t _budget;
			private:
				/**
				 * \~russian
				 * Наибольший допустимый объём работы сопоставления
				 *
				 * @details Потолок обрезает объём, вычисленный вызывающей стороною, и
				 *          устанавливается ею же: умолчанием служит «MAX_STEPS».
				 *
				 * \~english
				 * Largest admissible amount of work of the matching
				 * @details The ceiling truncates the amount computed by the calling side and
				 *          is set by it as well: «MAX_STEPS» serves as the default.
				 *
				 * \~
				 */
				size_t _ceiling;
			private:
				/**
				 * \~russian
				 * Наибольшее число попыток сопоставления
				 *
				 * @details Предел действует на одно последующее сопоставление и служит
				 *          вызывающей стороне, располагающей запасным способом: поиск,
				 *          предел исчерпавший, прекращается, а отказ отмечается особо -
				 *          отсутствием совпадения он не является.
				 *
				 * \~english
				 * Largest number of matching attempts
				 * @details The limit acts on one subsequent match and serves
				 *          a calling side that has a fallback way at its disposal: the search
				 *          that has exhausted the limit is stopped, while the failure is marked specially —
				 *          it is not an absence of a match.
				 *
				 * \~
				 */
				size_t _horizon;
			private:
				// Признак прекращения сопоставления пределом числа попыток
				bool _bounded;
			private:
				/**
				 * \~russian
				 * Позиция первой попытки сопоставления
				 *
				 * @details Позиция действует на одно последующее сопоставление и служит
				 *          вызывающей стороне, знающей, что левее неё совпадение не
				 *          начинается: попытки там уже сделаны и окончательно отказали.
				 *          Начала поиска она не меняет - привязка «\G» и признак
				 *          «ATSTART» по-прежнему судят о нём.
				 *
				 * \~english
				 * Position of the first matching attempt
				 * @details The position acts on one subsequent match and serves a calling side
				 *          that knows no match starts to the left of it: the attempts there have
				 *          already been made and have failed conclusively. It does not change
				 *          the start of the search - the «\G» anchor and the «ATSTART» flag
				 *          still judge by the latter.
				 *
				 * \~
				 */
				size_t _onset;
			private:
				// Позиция попытки, на которой сопоставление прекращено пределом
				size_t _frontier;
			private:
				// Действующий объём работы текущего сопоставления
				size_t _limit;
			private:
				// Наибольшая допустимая глубина рекурсивных вызовов подвыражений
				size_t _nesting;
			private:
				/**
				 * \~russian
				 * Действующая наибольшая глубина рекурсивных вызовов сопоставления
				 *
				 * @details Глубина берётся наименьшей из заданной вызывающей стороной
				 *          и заданной самим выражением указанием «(*LIMIT_DEPTH=N)»:
				 *          предел выражения понижает предел вызывающей стороны,
				 *          но не повышает его.
				 *
				 * \~english
				 * Effective largest depth of the recursive calls of the matching
				 * @details The depth is taken as the smallest of the one set by the calling side
				 *          and the one set by the expression itself by the «(*LIMIT_DEPTH=N)» option:
				 *          the limit of the expression lowers the limit of the calling side
				 *          but never raises it.
				 *
				 * \~
				 */
				size_t _deepest;
			private:
				/**
				 * \~russian
				 * Действующий наибольший объём памяти сопоставления в байтах
				 *
				 * @details Объём задаётся самим выражением указанием «(*LIMIT_HEAP=N)»
				 *          и считается по наборам точек возврата, кадров вызовов
				 *          и записей журнала наравне с прочими пределами их размеров.
				 *
				 * \~english
				 * Effective largest amount of the matching memory in bytes
				 * @details The amount is set by the expression itself by the «(*LIMIT_HEAP=N)» option
				 *          and is counted over the sets of the backtracking points, the call frames
				 *          and the journal entries along with the other limits of their sizes.
				 *
				 * \~
				 */
				size_t _memory;
			private:
				/**
				 * \~russian
				 * Вид глагола управления, попытку сопоставления прекратившего
				 *
				 * @details Значение выводится увеличенным на единицу наравне с точкой
				 *          возврата, а нуль означает прекращение обыкновенное. Внешний
				 *          обход позиций начала читает его и решает, продолжать ли
				 *          попытки: глагол отказа целиком их прекращает, а глаголы
				 *          переноса задают позицию продолжения.
				 *
				 * \~english
				 * Kind of the control verb that terminated the matching attempt
				 * @details The value is yielded increased by one along with the backtracking point,
				 *          whereas zero means an ordinary termination. The outer walk over the starting
				 *          positions reads it and decides whether to continue the attempts:
				 *          the verb of the whole refusal terminates them, whereas the verbs of moving
				 *          set the position of continuation.
				 *
				 * \~
				 */
				uint8_t _control;
			private:
				// Позиция продолжения попытки сопоставления глаголом переноса
				size_t _resume;
			private:
				/**
				 * \~russian
				 * Адрес глагола отметки, попыткою последней пройденного
				 *
				 * @details Ячейка отметки возвратом отменяется, отчего по отказу
				 *          сопоставления в ней ничего не остаётся. Эталонная же
				 *          реализация имя отметки выводит и при отказе - отметку
				 *          последнюю попытки последней, - и адрес ведётся потому
				 *          отдельно: возвратом он не отменяется, началом попытки
				 *          очищается, а глаголом отсечения снимается.
				 *
				 * \~english
				 * Address of the mark verb passed by the last attempt
				 * @details The mark cell is undone by backtracking, whereby nothing remains in it
				 *          upon a failure of the matching. The reference implementation, however,
				 *          yields the name of the mark upon a failure as well — the last mark
				 *          of the last attempt — and the address is therefore maintained
				 *          separately: it is not undone by backtracking, is cleared at the start
				 *          of an attempt and is removed by a cutting verb.
				 *
				 * \~
				 */
				size_t _failing;
			private:
				// Действующая вложенность исполнений программы
				size_t _nested;
			private:
				// Опознание программы, для какой построены таблицы принадлежности байтов
				uint64_t _identity;
			private:
				/**
				 * \~russian
				 * Номера таблиц принадлежности байтов по номерам классов программы
				 *
				 * @details Номер «INVALID_ADDRESS» означает, что таблица классу ещё
				 *          не заведена. Набор растёт до наибольшего номера класса,
				 *          пройденного повторением, а не до числа классов программы:
				 *          сброс его при смене программы обходится без обхода.
				 *
				 * \~english
				 * Numbers of the byte belonging tables by the numbers of the classes of the program
				 * @details The number «INVALID_ADDRESS» means that no table has been set up for
				 *          the class yet. The set grows up to the largest number of a class
				 *          walked by a repetition rather than to the number of classes of the
				 *          program: resetting it on a change of the program needs no walk.
				 *
				 * \~
				 */
				vector <uint32_t> _indexes;
			private:
				/**
				 * \~russian
				 * Таблицы принадлежности байтов классам, пройденным повторением
				 *
				 * @details Таблица заводится на КАЖДЫЙ класс, а не одна на класс последний
				 *          встреченный. Повторение «(?:[a-z]+/)+» компиляция разворачивает
				 *          в два вхождения класса, и всякое вхождение заводит собственную
				 *          запись набора классов. Таблица одна перестраивалась тогда при
				 *          каждом переходе между вхождениями - по 256 разборов класса
				 *          на перестройку, дважды за сопоставление, - и это давало 92%
				 *          времени сопоставления. Сличение толкователя с толкователем
				 *          эталона PCRE2 показало долю 0.04 на таком выражении, 0.59
				 *          с таблицей на каждый класс.
				 *
				 * \~english
				 * Byte belonging tables of the classes walked by a repetition
				 * @details A table is set up for EVERY class rather than one for the last encountered
				 *          class. The compilation unrolls the repetition «(?:[a-z]+/)+» into two
				 *          occurrences of the class, and every occurrence sets up its own record
				 *          of the set of classes. A single table was then rebuilt at every
				 *          transition between the occurrences — 256 class evaluations per
				 *          rebuild, twice per match, — and that took 92% of the matching time.
				 *          Comparing the interpreter with the interpreter of the PCRE2 reference
				 *          showed the ratio of 0.04 on such an expression, 0.59 with a table
				 *          for every class.
				 *
				 * \~
				 */
				vector <table_t> _tables;
			private:
				/**
				 * \~russian
				 * Разрешения таблиц принадлежности байтов по номерам классов
				 *
				 * @details Набор служит прямым путём от номера класса к таблице его.
				 *          Ведётся он рядом с набором номеров, а не взамен его:
				 *          номера переживают перемещение набора таблиц, адреса же
				 *          нет, и восстановление адресов идёт именно по номерам.
				 *
				 * \~english
				 * Resolutions of the byte belonging tables by the numbers of the classes
				 * @details The set serves as a direct way from the number of a class to its table.
				 *          It is kept next to the set of numbers rather than instead of it:
				 *          the numbers survive a move of the set of tables, whereas the addresses
				 *          do not, and the restoration of the addresses goes exactly by the numbers.
				 *
				 * \~
				 */
				vector <lookup_t> _lookup;
			private:
				// Набор точек возврата исполнения программы
				vector <point_t> _points;
			private:
				// Журнал изменений ячеек захвата групп
				vector <change_t> _journal;
			private:
				// Набор позиций захвата групп и ячеек состояния исполнения
				vector <size_t> _slots;
			private:
				/**
				 * \~russian
				 * Набор отметок состояния возврата атомарных конструкций
				 *
				 * @details Отметка сохраняет глубину набора точек возврата на входе
				 *          в атомарную конструкцию, благодаря чему её завершение
				 *          отказывается от точек возврата, накопленных внутри неё.
				 *
				 * \~english
				 * Set of the marks of the backtracking state of the atomic constructs
				 * @details A mark keeps the depth of the set of backtracking points at the entry
				 *          into an atomic construct, thanks to which its completion
				 *          gives up the backtracking points accumulated inside it.
				 *
				 * \~
				 */
				vector <size_t> _marks;
			private:
				/**
				 * \~russian
				 * @brief Изменение отметки атомарной группы
				 *
				 * @details Ячейка отметки одна на всю программу, а рекурсивный
				 *          вызов входит в ту же атомарную группу заново
				 *          и ячейку перезаписывает. Оттого отсечение уровня
				 *          внешнего брало глубину уровня внутреннего и точек
				 *          возврата не отсекало вовсе. Изменения ведутся
				 *          журналом наравне с ячейками захвата: кадр вызова
				 *          и точка возврата держат отсечку журнала, а возврат
				 *          восстанавливает прежние значения отметок.
				 *
				 * \~english
				 * @brief Change of an atomic group mark
				 * @details The mark cell is a single one for the whole program, while
				 *          a recursive call enters the same atomic group anew
				 *          and overwrites the cell. Because of that the cut of an outer level
				 *          took the depth of an inner level and cut no backtracking
				 *          points at all. Changes are kept in a journal alongside
				 *          the capture cells: the call frame and the backtracking point
				 *          keep the journal watermark, and backtracking
				 *          restores the previous values of the marks.
				 *
				 * \~
				 */
				typedef struct Remark {
					// Номер ячейки изменённой отметки
					uint32_t cell;
					// Прежнее значение отметки
					size_t value;
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
					Remark() noexcept : cell(0), value(0) {}
				} remark_t;
			private:
				// Журнал изменений отметок атомарных групп
				vector <remark_t> _remarks;
			private:
				/**
				 * \~russian
				 * @brief Кадр исполняемого рекурсивного вызова подвыражения
				 *
				 * \~english
				 * @brief Frame of an executed recursive call of a subexpression
				 *
				 * \~
				 */
				typedef struct Frame {
					// Адрес инструкции, к которой возвращается исполнение
					address_t back;
					// Номер группы, рекурсивный вызов которой исполняется
					uint32_t number;
					// Размер журнала изменений ячеек захвата на момент вызова
					size_t journal;
					// Размер журнала изменений отметок атомарных групп на момент вызова
					size_t remarks;
					// Номер кадра вызова, из которого выполнен рекурсивный вызов
					size_t parent;
					// Глубина рекурсивного вызова, отсчитываемая с единицы
					size_t depth;
					/**
					 * \~russian
					 * Позиция текста, в какой рекурсивный вызов начат
					 *
					 * @details Позиция служит распознаванию вызова, вошедшего заново
					 *          в той же позиции текста: такой вызов повторяет уже
					 *          выполняемое и завершиться не может.
					 *
					 * \~english
					 * Position of the text at which the recursive call was started
					 * @details The position serves to recognise a call that has entered anew
					 *          at the same position of the text: such a call repeats what is
					 *          already being executed and cannot complete.
					 *
					 * \~
					 */
					size_t pos;
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
					Frame() noexcept : back(0), number(0), journal(0), parent(0), depth(0), pos(0) {}
				} frame_t;
			private:
				// Набор кадров рекурсивных вызовов подвыражений
				vector <frame_t> _frames;
			private:
				// Номер кадра исполняемого рекурсивного вызова
				size_t _current;
			private:
				// Набор изменений ячеек захвата, отменяемых возвратом из вызова
				vector <change_t> _undo;
			private:
				// Код ошибки последней операции сопоставления
				error_t _error;
			private:
				/**
				 * \~russian
				 * Позиция вхождения обязательного литерала, вызывающей стороной найденная
				 *
				 * @details Позиция действует на одно последующее сопоставление, как и
				 *          позиция первой попытки. Поле стоит последним намеренно:
				 *          смещения полей, читаемых циклом исполнения, оно не сдвигает.
				 *
				 * \~english
				 * Position of the occurrence of the mandatory literal found by the calling side
				 * @details The position acts on one subsequent match, as the position of the
				 *          first attempt does. The field stands last deliberately: it does not shift
				 *          the offsets of the fields read by the execution loop.
				 *
				 * \~
				 */
				size_t _located;
			public:
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом
				 *
				 * @param program  исполняемая программа регулярного выражения
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against a text
				 * @param program  program of the regular expression being executed
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the match and of the captured groups
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(const program_t & program, string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом в заданном режиме
				 *
				 * @param program  исполняемая программа регулярного выражения
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @param mode     режим сопоставления регулярного выражения с текстом
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against a text in the given mode
				 * @param program  program of the regular expression being executed
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the match and of the captured groups
				 * @param mode     mode of matching the regular expression against the text
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(const program_t & program, string_view text, const size_t start, vector <pair <size_t, size_t>> & captures, const mode_t mode) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сопоставления символа одиночной инструкцией
				 *
				 * @details Метод применяется при проходе ряда подходящих символов
				 *          и сопоставляет лишь инструкции, продвигающиеся по тексту
				 *          независимо от состояния исполнения.
				 *
				 * @param instruction сопоставляющая инструкция программы
				 * @param pos         позиция сопоставления в тексте
				 * @param width       длина сопоставленного символа в байтах
				 * @return            результат сопоставления символа инструкцией
				 *
				 * \~english
				 * @brief Method of matching a character by a single instruction
				 * @details The method is used when walking a run of matching characters
				 *          and matches only the instructions that advance through the text
				 *          independently of the state of the execution.
				 * @param instruction matching instruction of the program
				 * @param pos         matching position in the text
				 * @param width       length of the matched character in bytes
				 * @return            result of matching the character by the instruction
				 *
				 * \~
				 */
				bool single(const instruction_t & instruction, const size_t pos, size_t & width) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки допустимого объёма работы сопоставления
				 *
				 * @details Установленный объём действует на одно последующее сопоставление,
				 *          после которого восстанавливается предельный. Уменьшенный объём
				 *          требуется вызывающей стороне, располагающей запасным способом
				 *          сопоставления: исчерпание объёма прекращает исполнение с ошибкой
				 *          «BUDGET_EXCEEDED», и сопоставление выполняется запасным способом,
				 *          время которого не зависит от вида выражения.
				 *
				 * @param budget допустимое количество шагов сопоставления
				 *
				 * \~english
				 * @brief Method of setting the admissible amount of work of the matching
				 * @details The set amount acts on one subsequent match,
				 *          after which the limiting one is restored. A reduced amount
				 *          is required by a calling side that has a fallback way of
				 *          matching at its disposal: exhausting the amount stops the execution with the
				 *          «BUDGET_EXCEEDED» error, and the matching is performed by the fallback way,
				 *          whose time does not depend on the kind of the expression.
				 * @param budget admissible number of matching steps
				 *
				 * \~
				 */
				void budget(const size_t budget) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки наибольшего числа попыток сопоставления
				 *
				 * @details Предел действует на одно последующее сопоставление, после
				 *          которого снимается. Требуется он вызывающей стороне, желающей
				 *          испытать исполнение с возвратом малою ценой, не платя за проход
				 *          всего текста: совпадение, столькими попытками не найденное,
				 *          отыскивается способом запасным.
				 *
				 *          Считаются попытки, а не позиции: отбор по обязательному литералу
				 *          перешагивает через текст целыми участками, и предел по позициям
				 *          отнимал бы у него ровно то, ради чего он заведён.
				 *
				 *          Предел этот НЕ равнозначен объёму работы: объём ограничивает
				 *          шаги, а одна попытка способна пройти весь текст единственным
				 *          повторением, отчего оба предела и ставятся вместе.
				 *
				 * @param horizon наибольшее число попыток сопоставления
				 *
				 * \~english
				 * @brief Method of setting the largest number of matching attempts
				 * @details The limit acts on one subsequent match, after which
				 *          it is removed. It is required by a calling side that wishes
				 *          to try the execution with backtracking at a small price without paying for
				 *          a pass over the whole text: a match not found in that many attempts
				 *          is located by the fallback way.
				 *          The attempts are counted rather than the positions: the selection by the mandatory literal
				 *          steps over whole stretches of the text, and a limit by the positions
				 *          would take away from it exactly what it is introduced for.
				 *          This limit is NOT equivalent to the amount of work: the amount bounds
				 *          the steps, while a single attempt is able to pass over the whole text by a single
				 *          repetition, which is why both limits are set together.
				 * @param horizon largest number of matching attempts
				 *
				 * \~
				 */
				void horizon(const size_t horizon) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения признака прекращения пределом числа попыток
				 *
				 * @details Признак отличает отказ по пределу числа попыток от отсутствия
				 *          совпадения: первый требует запасного способа, второй
				 *          окончателен.
				 *
				 * @return признак прекращения сопоставления пределом числа попыток
				 *
				 * \~english
				 * @brief Method of getting the indication of a stop by the limit of the attempts
				 * @details The indication distinguishes a failure by the limit of the attempts from an absence
				 *          of a match: the first requires the fallback way, the second
				 *          is final.
				 * @return indication of a stop of the matching by the limit of the attempts
				 *
				 * \~
				 */
				bool bounded() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки позиции первой попытки сопоставления
				 *
				 * @details Позиция действует на одно последующее сопоставление, после
				 *          чего снимается. Она нужна вызывающей стороне, повторяющей поиск,
				 *          прежде прерванный пределом: попытки левее позиции, на которой
				 *          предел его прервал, окончательно отказали, и повторять их
				 *          незачем. Начало поиска при этом остаётся прежним - о нём судят
				 *          привязка «\G» и признак «ATSTART», - а у выражения, к началу
				 *          поиска привязанного, позиция не действует вовсе: попытка у него
				 *          единственная.
				 *
				 * @param onset позиция первой попытки сопоставления
				 *
				 * \~english
				 * @brief Method of setting the position of the first matching attempt
				 * @details The position acts on one subsequent match, after which it is
				 *          removed. It is needed by a calling side repeating a search previously
				 *          interrupted by a limit: the attempts to the left of the position where
				 *          the limit interrupted it have failed conclusively, and there is no need
				 *          to repeat them. The start of the search stays the same - the «\G»
				 *          anchor and the «ATSTART» flag judge by it, - and for an expression
				 *          anchored to the start of the search the position does not act at all:
				 *          it has a single attempt.
				 * @param onset position of the first matching attempt
				 *
				 * \~
				 */
				void onset(const size_t onset) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод передачи вхождения обязательного литерала
				 *
				 * @details Позиция действует на одно последующее сопоставление, после
				 *          чего снимается. Её передаёт движок, проверивший возможность
				 *          совпадения до выбора пути исполнения: найденное проверкой
				 *          вхождение исполнение берёт как есть, а не разыскивает заново.
				 *          Вхождение обязано быть первым не левее начала поиска; лежащее
				 *          левее первой попытки исполнение отбрасывает и ищет литерал
				 *          от неё само.
				 *
				 * @param located позиция вхождения обязательного литерала либо «npos»
				 *
				 * \~english
				 * @brief Method of handing over the occurrence of the mandatory literal
				 * @details The position acts on one subsequent match, after which it is
				 *          removed. It is handed over by the engine that checked the possibility
				 *          of a match before choosing the path of execution: the execution takes
				 *          the occurrence found by the check as is instead of searching for it anew.
				 *          The occurrence must be the first one not to the left of the start of the
				 *          search; one lying to the left of the first attempt the execution discards
				 *          and searches for the literal from the attempt by itself.
				 * @param located position of the occurrence of the mandatory literal or «npos»
				 *
				 * \~
				 */
				void located(const size_t located) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения позиции, на которой сопоставление прекращено пределом
				 *
				 * @details Позиция принадлежит попытке, пределом прерванной либо до
				 *          предела не допущенной: все попытки левее неё окончательно
				 *          отказали. Сопоставление, пределом не прерванное, позиции
				 *          не устанавливает.
				 *
				 * @return позиция прекращения сопоставления либо «npos»
				 *
				 * \~english
				 * @brief Method of getting the position where the matching was stopped by a limit
				 * @details The position belongs to the attempt interrupted by the limit or not
				 *          admitted before it: all the attempts to the left of it have failed
				 *          conclusively. A matching not interrupted by a limit sets no position.
				 * @return position of the stop of the matching or «npos»
				 *
				 * \~
				 */
				size_t frontier() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки наибольшего допустимого объёма работы сопоставления
				 *
				 * @details Потолок обрезает объём, вычисленный вызывающей стороною от длины
				 *          текста и длины программы, и действует на все последующие
				 *          сопоставления, а не на одно. Нуль потолок снимает вовсе.
				 *
				 * @param ceiling наибольшее допустимое количество шагов сопоставления
				 *
				 * \~english
				 * @brief Method of setting the largest admissible amount of work of the matching
				 * @details The ceiling truncates the amount computed by the calling side from the length
				 *          of the text and the length of the program, and acts on all subsequent
				 *          matches rather than on one. Zero removes the ceiling entirely.
				 * @param ceiling largest admissible number of matching steps
				 *
				 * \~
				 */
				void ceiling(const size_t ceiling) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки наибольшей допустимой глубины рекурсивных вызовов
				 *
				 * @details Глубина ограничивает вложенность рекурсивных вызовов подвыражений:
				 *          память под кадры вызовов растёт соразмерно ей. Нуль восстанавливает
				 *          глубину умолчания.
				 *
				 * @param nesting наибольшая допустимая глубина рекурсивных вызовов
				 *
				 * \~english
				 * @brief Method of setting the largest admissible depth of recursive calls
				 * @details The depth bounds the nesting of recursive calls of subpatterns:
				 *          the memory of the call frames grows in proportion to it. Zero
				 *          restores the depth of the default.
				 * @param nesting largest admissible depth of recursive calls
				 *
				 * \~
				 */
				void nesting(const size_t nesting) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки последней операции
				 *
				 * @details Код ошибки «BUDGET_EXCEEDED» означает исчерпание допустимого
				 *          объёма работы сопоставления, при котором отсутствие совпадения
				 *          не установлено.
				 *
				 * @return код ошибки последней операции сопоставления
				 *
				 * \~english
				 * @brief Method of getting the error code of the last operation
				 * @details The «BUDGET_EXCEEDED» error code means the exhaustion of the admissible
				 *          amount of work of the matching, at which the absence of a match
				 *          is not established.
				 * @return error code of the last matching operation
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения адреса глагола отметки совпадения последнего
				 *
				 * @details Адрес берётся из ячейки отметки последней, ведомой наравне
				 *          с ячейками захвата: по совпадении в ней остаётся глагол
				 *          пути, совпадение давшего. Значение недостижимое означает
				 *          совпадение, глаголов отметки не прошедшее.
				 *
				 * @return адрес глагола отметки совпадения последнего
				 *
				 * \~english
				 * @brief Method of getting the address of the mark verb of the last match
				 * @details The address is taken from the cell of the last mark, maintained along
				 *          with the capture cells: upon a match it holds the verb of the path
				 *          that produced the match. An unreachable value means a match
				 *          that passed no mark verbs.
				 * @return address of the mark verb of the last match
				 *
				 * \~
				 */
				size_t marked() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения адреса глагола отметки попытки последней
				 *
				 * @return адрес глагола отметки попытки последней
				 *
				 * \~english
				 * @brief Method of getting the address of the mark verb of the last attempt
				 * @return address of the mark verb of the last attempt
				 *
				 * \~
				 */
				size_t failed() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выполнения попытки сопоставления с заданной позиции
				 *
				 * @details Попытка исполняет программу с её первой инструкции, сохраняя
				 *          точки возврата и возвращаясь к ним при отказе сопоставления.
				 *          Исчерпание точек возврата означает отказ попытки.
				 *
				 * @param pos позиция начала попытки сопоставления
				 * @return    результат выполнения попытки сопоставления
				 *
				 * \~english
				 * @brief Method of performing a match attempt from the given position
				 * @details The attempt executes the program from its first instruction, saving
				 *          backtracking points and returning to them on a matching failure.
				 *          Exhausting the backtracking points means the failure of the attempt.
				 * @param pos position where the match attempt begins
				 * @return    result of performing the match attempt
				 *
				 * \~
				 */
				bool attempt(const size_t pos) noexcept;
				/**
				 * \~russian
				 * @brief Метод исполнения программы с заданной инструкции
				 *
				 * @details Исполнение сохраняет точки возврата в общем наборе, не опускаясь
				 *          ниже заданной глубины, благодаря чему вложенные исполнения
				 *          проверок окружения и рекурсивных вызовов не расходуют памяти
				 *          на собственные наборы. Завершение исполнения отказывается
				 *          от накопленных точек возврата, что соответствует запрету
				 *          возврата внутрь проверок окружения и рекурсивных вызовов.
				 *
				 * @param pc    адрес инструкции, с которой начинается исполнение
				 * @param pos   позиция в тексте, с которой начинается исполнение
				 * @param base  глубина набора точек возврата, ниже которой возврат недопустим
				 * @param bound позиция, в которой обязано завершиться исполнение
				 * @param end   позиция завершения исполнения программы
				 * @return      результат исполнения программы
				 *
				 * \~english
				 * @brief Method of executing the program from the given instruction
				 * @details The execution saves the backtracking points in a common set without descending
				 *          below the given depth, thanks to which the nested executions
				 *          of lookarounds and of recursive calls spend no memory
				 *          on sets of their own. Finishing the execution gives up
				 *          the accumulated backtracking points, which corresponds to the prohibition
				 *          of backtracking into lookarounds and recursive calls.
				 * @param pc    address of the instruction the execution starts from
				 * @param pos   position in the text the execution starts from
				 * @param base  depth of the set of backtracking points below which backtracking is inadmissible
				 * @param bound position at which the execution is obliged to finish
				 * @param end   position where the execution of the program finishes
				 * @return      result of executing the program
				 *
				 * \~
				 */
				bool run(const address_t pc, const size_t pos, const size_t base, const size_t bound, size_t & end) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления текста, захваченного группой
				 *
				 * @param number номер группы, захваченный текст которой сопоставляется
				 * @param flags  набор режимов компиляции инструкции
				 * @param pos    позиция сопоставления в тексте
				 * @param length длина сопоставленного захваченного текста
				 * @return       результат сопоставления захваченного текста
				 *
				 * \~english
				 * @brief Method of matching the text captured by a group
				 * @param number number of the group whose captured text is matched
				 * @param flags  set of compilation modes of the instruction
				 * @param pos    matching position in the text
				 * @param length length of the matched captured text
				 * @return       result of matching the captured text
				 *
				 * \~
				 */
				bool matches(const uint32_t number, const uint32_t flags, const size_t pos, size_t & length) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сохранения позиции в ячейке захвата
				 *
				 * @details Прежнее значение ячейки размещается в журнале изменений,
				 *          благодаря чему возврат восстанавливает состояние захвата.
				 *
				 * @param slot  номер ячейки захвата
				 * @param value сохраняемая в ячейке захвата позиция в тексте
				 *
				 * \~english
				 * @brief Method of saving a position in a capture cell
				 * @details The former value of the cell is placed in the change log,
				 *          thanks to which backtracking restores the state of the capture.
				 * @param slot  number of the capture cell
				 * @param value position in the text saved in the capture cell
				 *
				 * \~
				 */
				void store(const uint32_t slot, const size_t value) noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления состояния захвата групп
				 *
				 * @param mark размер журнала изменений, до которого выполняется откат
				 *
				 * \~english
				 * @brief Method of restoring the state of the group capture
				 * @param mark size of the change log the rollback is performed down to
				 *
				 * \~
				 */
				void restore(const size_t mark) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отката изменений отметок атомарных групп
				 *
				 * @details Откат ведётся по возврате из рекурсивного вызова
				 *          и по отмене его: ячейка отметки одна на всю программу,
				 *          и вызов, вошедший в ту же атомарную группу, значение
				 *          её перезаписывает.
				 *
				 * @param mark отсечка журнала изменений отметок
				 *
				 * \~english
				 * @brief Method of rolling back the changes of the atomic group marks
				 * @details The rollback is performed on returning from a recursive call
				 *          and on cancelling it: the mark cell is a single one for the whole program,
				 *          and a call that has entered the same atomic group overwrites
				 *          its value.
				 * @param mark watermark of the journal of the mark changes
				 *
				 * \~
				 */
				void rollback(const size_t mark) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод извлечения таблицы принадлежности байтов классу символов
				 *
				 * @details Таблица строится при первом обращении к классу и удерживается
				 *          до смены программы, а при смене набора режимов перестраивается.
				 *
				 * @param instruction инструкция класса символов, повторением проходимого
				 * @return            таблица принадлежности значений байта классу
				 *
				 * \~english
				 * @brief Method of getting the byte belonging table of a character class
				 * @details The table is built on the first reference to the class and is held
				 *          until the program changes, while a change of the set of modes rebuilds it.
				 * @param instruction instruction of the character class walked by a repetition
				 * @return            table of the belonging of the byte values to the class
				 *
				 * \~
				 */
				const uint8_t * table(const instruction_t & instruction) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод прохода ряда класса символов по таблице принадлежности байтов
				 *
				 * @param instruction инструкция сопоставления класса символов
				 * @param from        позиция начала прохода в тексте сопоставления
				 * @param size        размер текста сопоставления
				 * @param series      количество копий ряда, поглощению подлежащих
				 * @return            количество копий ряда, проходом поглощённых
				 *
				 * \~english
				 * @brief Method of walking a row of a character class by the byte belonging table
				 * @param instruction instruction matching a character class
				 * @param from        position of the start of the walk in the matching text
				 * @param size        size of the matching text
				 * @param series      number of the copies of the row subject to consumption
				 * @return            number of the copies of the row consumed by the walk
				 *
				 * \~
				 */
				size_t consume(const instruction_t & instruction, const size_t from, const size_t size, const uint16_t series) noexcept;
				/**
				 * \~russian
				 * @brief Метод сличения литерала с текстом одним заходом
				 *
				 * @details Первый символ литерала уже сопоставлен, и сличаются байты
				 *          за ним. Сличение останавливается на первом байте, литералу
				 *          не отвечающем, и на конце текста.
				 *
				 * @param instruction инструкция одиночного символа, литерал возглавляющая
				 * @param from        позиция первого символа литерала в тексте сопоставления
				 * @param size        размер текста сопоставления
				 * @return            количество символов литерала, с текстом совпавших подряд,
				 *                    с прибавкой BROKEN при обрыве на байте несовпавшем
				 *
				 * \~english
				 * @brief Method of comparing a literal with the text in one trip
				 * @details The first character of the literal is already matched, and the bytes after
				 *          it are compared. The comparison stops at the first byte not matching the
				 *          literal and at the end of the text.
				 * @param instruction instruction of a single character heading the literal
				 * @param from        position of the first character of the literal in the matching text
				 * @param size        size of the matching text
				 * @return            number of characters of the literal that coincided with the text in a row,
				 *                    increased by BROKEN on a break on a non-matching byte
				 *
				 * \~
				 */
				size_t literal(const instruction_t & instruction, const size_t from, const size_t size) const noexcept;
				/**
				 * \~russian
				 * @brief Метод прохода цепочки ограниченного повторения одиночного символа
				 *
				 * @details Цепочка проходится одним ходом и точкой возврата единственной
				 *          взамен двух инструкций и точки на каждую копию. Проход берётся
				 *          лишь у тела из класса символов вне режима разбора UTF-8; тело
				 *          иное проходится по инструкциям, о чём и говорит выдача
				 *
				 * @note Метод отделён от цикла исполнения намеренно и отделён замером:
				 *       внесённый телом цикла, он обобрал соседей своих по единице
				 *       трансляции на четверть скорости
				 *
				 * @param instruction переход, цепочку возглавляющий
				 * @param pc          адрес перехода в программе регулярного выражения
				 * @param from        позиция начала прохода в тексте сопоставления
				 * @param size        размер текста сопоставления
				 * @return            количество копий цепочки, проходом поглощённых
				 *
				 * \~english
				 * @brief Method of walking a chain of a bounded repetition of a single character
				 * @details The chain is walked in one move and with a single backtracking point
				 *          instead of two instructions and a point per every copy. The walk is
				 *          taken only for a body of a character class outside the UTF-8 parsing
				 *          mode; another body is walked by instructions, which is what the return
				 *          value tells
				 * @note The method is separated from the execution loop deliberately, and it is
				 *       separated by measurement: placed into the body of the loop, it robbed
				 *       its neighbours in the translation unit of a quarter of their speed
				 * @param instruction the jump heading the chain
				 * @param pc          address of the jump in the program of the regular expression
				 * @param from        position of the start of the walk in the matching text
				 * @param size        size of the matching text
				 * @return            number of the copies of the chain consumed by the walk
				 *
				 * \~
				 */
				size_t chain(const instruction_t & instruction, const address_t pc, const size_t from, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод построения таблицы принадлежности байтов классу символов
				 *
				 * @details Метод заводит таблицу классу, её не имеющему, строит её обходом
				 *          пространства значений байта и обновляет набор разрешений целиком.
				 *          Вынесен он из «table» отдельно затем, чтобы путь частый - выдача
				 *          таблицы разрешённой - остался коротким и встраиваемым.
				 *
				 * @param instruction инструкция класса символов, повторением проходимого
				 * @return            таблица принадлежности значений байта классу
				 *
				 * \~english
				 * @brief Method of building the byte belonging table of a character class
				 * @details The method sets up a table for a class that has none, builds it by
				 *          walking the space of the byte values and updates the set of resolutions
				 *          as a whole. It is separated out of «table» so that the frequent path —
				 *          issuing a resolved table — stays short and inlinable.
				 * @param instruction instruction of the character class walked by a repetition
				 * @return            table of the belonging of the byte values to the class
				 *
				 * \~
				 */
				const uint8_t * tabulate(const instruction_t & instruction) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принадлежности символа классу символов
				 *
				 * @details Значение, в один байт укладывающееся, проверяется таблицей,
				 *          а значение большее - вычислением: таблица покрывает лишь
				 *          двести пятьдесят шесть первых кодовых значений, а сложение
				 *          её на всю область Юникода стоило бы больше всякой выгоды.
				 *
				 * @param instruction инструкция класса символов
				 * @param code        проверяемое кодовое значение символа
				 * @return            результат проверки принадлежности символа классу
				 *
				 * \~english
				 * @brief Method of checking the belonging of a character to a character class
				 * @details A value that fits into a single byte is checked by the table, while
				 *          a greater value is checked by computation: the table covers only
				 *          the first two hundred and fifty six code values, and building it
				 *          for the whole Unicode area would cost more than any gain.
				 * @param instruction instruction of the character class
				 * @param code        checked code value of the character
				 * @return            result of the check of the belonging of the character to the class
				 *
				 * \~
				 */
				bool member(const instruction_t & instruction, const uint32_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод продвижения ленивого повторения одиночного символа
				 *
				 * @details Ленивый ряд сопоставление продолжает сразу, а тело повторения
				 *          поглощает по символу лишь по отказу продолжения. Продолжение,
				 *          начинаемое сопоставлением одиночного символа, отвергает
				 *          всякое положение, где текст несёт байт иной, и ряд продвигается
				 *          сразу к ближайшему такому байту взамен возврата в продолжение
				 *          на каждом символе. Положения, рядом пропускаемые, поверяются
				 *          принадлежностью телу повторения: ряд за байт, телу чуждый,
				 *          не проходит. Условие применимости дословно повторяет условие
				 *          порождателя кода - пути обязаны пропускать одни и те же
				 *          положения, иначе сличение их разошлось бы захватами.
				 *
				 * @param instruction инструкция перехода, ленивое повторение возглавляющая
				 * @param pos         позиция ряда, продвижением изменяемая
				 * @param reached     признак достижения положения, продолжению пригодного
				 * @return            результат применимости продвижения ряда
				 *
				 * \~english
				 * @brief Method of the advancement of a lazy repetition of a single character
				 * @details A lazy row continues the matching at once, while the body of the
				 *          repetition consumes a character only upon a refusal of the continuation.
				 *          A continuation begun by the matching of a single character rejects
				 *          every position where the text carries a different byte, and the row
				 *          advances straight to the nearest such byte instead of returning into
				 *          the continuation at every character. The positions skipped by the row
				 *          are verified by the belonging to the body of the repetition: the row
				 *          does not pass a byte foreign to the body. The condition of applicability
				 *          repeats the condition of the code generator word for word — the paths
				 *          must skip the same positions, otherwise their comparison would diverge
				 *          in the captures.
				 * @param instruction instruction of the branch heading the lazy repetition
				 * @param pos         position of the row changed by the advancement
				 * @param reached     flag of reaching a position suitable for the continuation
				 * @return            result of the applicability of the advancement of the row
				 *
				 * \~
				 */
				bool advance(const instruction_t & instruction, size_t & pos, bool & reached) noexcept;
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
				Backtrack() noexcept;
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
				~Backtrack() noexcept {}
		} backtrack_t;
	};
};
