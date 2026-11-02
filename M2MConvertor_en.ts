<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en">
<context>
    <name>App</name>
    <message>
        <location filename="mainwindow.cpp" line="69"/>
        <source>M2MConvertor — эквивалентные автоматы</source>
        <translation>M2MConvertor — equivalent automata</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="136"/>
        <source>Преобразование конечных автоматов Мили и Мура</source>
        <translation>Mealy and Moore automata conversion</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="146"/>
        <source>1. База вариантов</source>
        <translation>1. Variant archive</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="170"/>
        <source>2. Вариант работы</source>
        <translation>2. Assignment variant</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="184"/>
        <source>—</source>
        <translation>—</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="185"/>
        <source>Вариант:</source>
        <translation>Variant:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="186"/>
        <source>Преобразование:</source>
        <translation>Conversion:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="189"/>
        <location filename="mainwindow.cpp" line="191"/>
        <source>Создать работу</source>
        <translation>Start assignment</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="214"/>
        <source>К выбору варианта</source>
        <translation>Back to variants</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="227"/>
        <source>Начало работы</source>
        <translation>Getting started</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="232"/>
        <source>Автомат Мура</source>
        <translation>Moore automaton</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="233"/>
        <source>Автомат Мили</source>
        <translation>Mealy automaton</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="234"/>
        <source>Отчёт</source>
        <translation>Report</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="138"/>
        <source>Количество состояний:</source>
        <translation>Number of states:</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="139"/>
        <source>Количество входных сигналов:</source>
        <translation>Number of input signals:</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="140"/>
        <source>Количество выходных сигналов:</source>
        <translation>Number of output signals:</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="154"/>
        <location filename="ui/automatoneditor.cpp" line="225"/>
        <source>Принять</source>
        <translation>Accept</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="279"/>
        <location filename="ui/automatoneditor.cpp" line="195"/>
        <source>Задание варианта</source>
        <translation>Variant task</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="281"/>
        <source>Пояснения к работе</source>
        <translation>Work instructions</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="290"/>
        <source>Преподаватель</source>
        <translation>Teacher</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="293"/>
        <source>Ф. И. О. преподавателя:</source>
        <translation>Teacher&apos;s full name:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="294"/>
        <source>Студент</source>
        <translation>Student</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="299"/>
        <source>Ф. И. О. студента:</source>
        <translation>Student&apos;s full name:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="300"/>
        <source>Группа:</source>
        <translation>Group:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="301"/>
        <source>Номер зачётной книжки:</source>
        <translation>Student record book number:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="302"/>
        <source>Создать отчёт HTML…</source>
        <translation>Create HTML report…</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="321"/>
        <source>Файл</source>
        <translation>File</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="322"/>
        <source>Выбрать другой вариант</source>
        <translation>Choose another variant</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="194"/>
        <location filename="mainwindow.cpp" line="195"/>
        <location filename="mainwindow.cpp" line="323"/>
        <source>Открыть работу…</source>
        <translation>Open work…</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="153"/>
        <source>Откройте архив .m2m</source>
        <translation>Open an .m2m archive</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="264"/>
        <source>Изучите изображение задания. Нажмите «Продолжить», чтобы перейти к таблице автомата Мура.

В таблице Мура укажите количества состояний, входных и выходных сигналов и примите размеры. Заполните входы x0…x9, выходы y0…y9 и переходы между состояниями a0, a1, …; a0 всегда начальное. «-» обозначает отсутствие перехода или выхода.

Принимайте представления по порядку: таблица, матрица A и граф Мура, затем таблица, матрица B и граф эквивалентного автомата Мили. Следующий этап откроется после правильного выполнения предыдущего.

Каждое представление проверяется по реакции варианта из a0. Для Мура сравнивается выход до перехода, для Мили — выход на переходе. Совпадение конечной реакции не доказывает эквивалентность на всех входных последовательностях.

Подсказку можно уменьшить или скрыть разделителем. Примеры ввода доступны при наведении на ячейки. Ctrl+S сохраняет работу; в конце заполните сведения об участниках и создайте отчёт.</source>
        <translation>Study the task image. Click Continue to open the Moore automaton table.

In the Moore table, enter the numbers of states, inputs and outputs, then accept the dimensions. Fill in inputs x0…x9, outputs y0…y9 and transitions between states a0, a1, …; a0 is always the initial state. “-” means no transition or output.

Accept each representation in order: the Moore table, matrix A and graph, followed by the table, matrix B and graph of the equivalent Mealy automaton. Each stage opens after the previous one is completed correctly.

Each representation is checked against the variant’s reaction starting from a0. Moore outputs are compared before transitions; Mealy outputs are compared on transitions. Matching a finite reaction does not prove equivalence for every input sequence.

Use the divider to resize or hide the hint. Hover over cells for input examples. Ctrl+S saves your work; at the end, fill in the participant details and create a report.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="276"/>
        <source>Продолжить</source>
        <translation>Continue</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="324"/>
        <source>Сохранить работу</source>
        <translation>Save work</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="327"/>
        <source>Сохранить работу как…</source>
        <translation>Save work as…</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="329"/>
        <source>Выход</source>
        <translation>Exit</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="332"/>
        <source>Настройки</source>
        <translation>Settings</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="162"/>
        <location filename="mainwindow.cpp" line="163"/>
        <source>Открыть архив вариантов…</source>
        <translation>Open variant archive…</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="177"/>
        <source>Выберите номер варианта.</source>
        <translation>Select a variant number.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="334"/>
        <source>Журнал событий</source>
        <translation>Event log</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="338"/>
        <source>Язык</source>
        <translation>Language</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="420"/>
        <source>Мили → Мура — в разработке. Выберите вариант Мура → Мили.</source>
        <translation>Mealy → Moore is under development. Select a Moore → Mealy variant.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="56"/>
        <source>Таблица</source>
        <translation>Table</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="57"/>
        <source>Матрица</source>
        <translation>Matrix</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="58"/>
        <source>Граф</source>
        <translation>Graph</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="115"/>
        <source>Очистить таблицу</source>
        <translation>Clear table</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="174"/>
        <source>Мили: ячейка «следующее состояние / выход1, выход2». Мура: выходы над состояниями. Введите входы x0…x9 слева; «-» означает отсутствие перехода или выхода.</source>
        <translation>Mealy: enter “next state / output1, output2”. Moore: outputs are above the states. Enter inputs x0…x9 on the left; “-” means no transition or output.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="178"/>
        <source>Принять таблицу</source>
        <translation>Accept table</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="212"/>
        <source>Очистить матрицу</source>
        <translation>Clear matrix</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="224"/>
        <source>Размерность матрицы</source>
        <translation>Matrix dimension</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="228"/>
        <source>Размерность:</source>
        <translation>Dimensions:</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="251"/>
        <location filename="ui/automatoneditor.cpp" line="1125"/>
        <source>Состояние</source>
        <translation>State</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="251"/>
        <location filename="ui/automatoneditor.cpp" line="426"/>
        <location filename="ui/automatoneditor.cpp" line="1121"/>
        <location filename="ui/automatoneditor.cpp" line="1125"/>
        <source>Выходные сигналы</source>
        <translation>Output signals</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="263"/>
        <source>В каждой ячейке укажите метки дуг через «;». Для Мили: «вход / выход1,выход2»; для Мура: «x1, x2». Строка — откуда, столбец — куда. Выходы Мура заполните в таблице справа.</source>
        <translation>Separate multiple edge labels with “;”. Mealy: “input / output1,output2”; Moore: “x1, x2”. Rows are source states, columns are destinations. Enter Moore outputs in the table on the right.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="267"/>
        <source>Принять матрицу</source>
        <translation>Accept matrix</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="289"/>
        <location filename="ui/automatoneditor.cpp" line="350"/>
        <source>Принятая таблица автомата</source>
        <translation>Accepted automaton table</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="306"/>
        <source>Создать вершину</source>
        <translation>Create vertex</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="308"/>
        <source>Удалить последнюю вершину</source>
        <translation>Delete last vertex</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="331"/>
        <source>Двойной щелчок изменяет выходы вершины Мура или подпись дуги. Выберите исходную вершину, затем удерживайте Ctrl и щёлкните по конечной вершине — появится дуга. Backspace/Delete удаляет последнюю вершину (выбранную дугу можно удалить отдельно).</source>
        <translation>Double-click to edit Moore vertex outputs or an edge label. Select a source vertex, then Ctrl-click a destination vertex to create an edge. Backspace/Delete removes the last vertex (a selected edge can be deleted separately).</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="337"/>
        <source>Принять граф</source>
        <translation>Accept graph</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="423"/>
        <location filename="ui/automatoneditor.cpp" line="436"/>
        <location filename="ui/automatoneditor.cpp" line="1119"/>
        <location filename="ui/automatoneditor.cpp" line="1123"/>
        <source>Входные сигналы</source>
        <translation>Input signals</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="430"/>
        <location filename="ui/automatoneditor.cpp" line="1122"/>
        <source>Состояния</source>
        <translation>States</translation>
    </message>
    <message>
        <location filename="ui/mealycelldelegate.cpp" line="28"/>
        <source>Следующее состояние: одно a0, a1, … или -</source>
        <translation>Next state: one a0, a1, … or -</translation>
    </message>
    <message>
        <location filename="ui/mealycelldelegate.cpp" line="29"/>
        <source>Выходы: y0…y9 через запятую или -</source>
        <translation>Outputs: y0…y9 separated by commas, or -</translation>
    </message>
    <message>
        <location filename="ui/widgets/hintsplitter.cpp" line="16"/>
        <source>Размер подсказки</source>
        <translation>Hint size</translation>
    </message>
    <message>
        <location filename="ui/widgets/hintsplitter.cpp" line="17"/>
        <source>Потяните за границу, что бы изменить размер области</source>
        <translation>Drag the divider to resize the area.</translation>
    </message>
    <message>
        <location filename="ui/widgets/hintsplitter.cpp" line="78"/>
        <source>Рабочая область</source>
        <translation>Work area</translation>
    </message>
</context>
<context>
    <name>AutomatonGraphEditor</name>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="435"/>
        <source>Выходы состояния %1</source>
        <translation>Outputs of state %1</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="436"/>
        <source>Выходы y0…y9 через запятую, либо «-»:</source>
        <translation>Outputs y0…y9 separated by commas, or “-”:</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="447"/>
        <source>Вход / выходы; например x1 / y1 или x1 / -; x2 / y2</source>
        <translation>Input / outputs; for example x1 / y1 or x1 / -; x2 / y2</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="448"/>
        <source>Входы через запятую, например x1, x2</source>
        <translation>Inputs separated by commas, for example x1, x2</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="449"/>
        <source>Подпись дуги</source>
        <translation>Edge label</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="584"/>
        <source>Повреждены данные графического редактора.</source>
        <translation>The graph editor data is corrupted.</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="585"/>
        <source>Допускается не более 10 вершин.</source>
        <translation>No more than 10 vertices are allowed.</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="589"/>
        <location filename="ui/automatongrapheditor.cpp" line="594"/>
        <source>Повреждена вершина сохранённого графа.</source>
        <translation>A vertex in the saved graph is corrupted.</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="605"/>
        <source>Повреждена дуга сохранённого графа.</source>
        <translation>An edge in the saved graph is corrupted.</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="610"/>
        <source>Дуга сохранённого графа ссылается на неизвестную вершину.</source>
        <translation>An edge in the saved graph refers to an unknown vertex.</translation>
    </message>
</context>
<context>
    <name>Legacy</name>
    <message>
        <location filename="domain/conversionservice.cpp" line="90"/>
        <source>Не удалось построить состояние автомата Мура.</source>
        <translation>Could not construct a Moore state.</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="117"/>
        <source>Эталонный автомат некорректен: %1</source>
        <translation>The reference automaton is invalid: %1</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="119"/>
        <location filename="domain/isomorphismchecker.cpp" line="166"/>
        <source>Введённый автомат некорректен: %1</source>
        <translation>The entered automaton is invalid: %1</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="121"/>
        <source>Не совпадает число входных сигналов.</source>
        <translation>The numbers of input signals do not match.</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="123"/>
        <source>Не совпадает число выходных сигналов.</source>
        <translation>The numbers of output signals do not match.</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="128"/>
        <source>Автоматы неэквивалентны: после удаления недостижимых состояний и минимизации получено разное число различимых состояний (%1 и %2).</source>
        <translation>The automata are not equivalent: removing unreachable states and minimizing produced different numbers of distinguishable states (%1 and %2).</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="135"/>
        <source>Автоматы неэквивалентны: их реакции различаются хотя бы для одной входной последовательности.</source>
        <translation>The automata are not equivalent: their reactions differ for at least one input sequence.</translation>
    </message>
    <message>
        <location filename="domain/equivalencechecker.cpp" line="138"/>
        <source>Верно. Автоматы поведенчески эквивалентны для всех входных последовательностей.</source>
        <translation>Correct. The automata are behaviorally equivalent for all input sequences.</translation>
    </message>
    <message>
        <location filename="domain/isomorphismchecker.cpp" line="168"/>
        <source>Выбран неверный тип автомата.</source>
        <translation>The selected automaton type is incorrect.</translation>
    </message>
    <message>
        <location filename="domain/isomorphismchecker.cpp" line="173"/>
        <source>Не совпадает число состояний, сигналов или переходов.</source>
        <translation>The numbers of states, signals or transitions do not match.</translation>
    </message>
    <message>
        <location filename="domain/isomorphismchecker.cpp" line="176"/>
        <source>Нагруженные графы не изоморфны: проверьте начальное состояние, переходы и выходы.</source>
        <translation>The labeled graphs are not isomorphic: check the initial state, transitions and outputs.</translation>
    </message>
    <message>
        <location filename="domain/isomorphismchecker.cpp" line="178"/>
        <source>Верно. Автоматы изоморфны с точностью до переименования состояний и сигналов.</source>
        <translation>Correct. The automata are isomorphic up to renaming states and signals.</translation>
    </message>
</context>
<context>
    <name>LogWindow</name>
    <message>
        <location filename="ui/logwindow.ui" line="6"/>
        <source>Application Log</source>
        <translation>Application Log</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="22"/>
        <source>Info</source>
        <translation>Info</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="32"/>
        <source>Warnings</source>
        <translation>Warnings</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="42"/>
        <source>Errors</source>
        <translation>Errors</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="52"/>
        <source>Auto-scroll</source>
        <translation>Auto-scroll</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="76"/>
        <source>Copy Selected</source>
        <translation>Copy Selected</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="83"/>
        <source>Clear Log</source>
        <oldsource>Clear View</oldsource>
        <translation>Clear Log</translation>
    </message>
    <message>
        <location filename="ui/logwindow.ui" line="90"/>
        <source>Save to File...</source>
        <translation>Save to File...</translation>
    </message>
    <message>
        <location filename="ui/logwindow.cpp" line="86"/>
        <source>Save Log</source>
        <translation>Save Log</translation>
    </message>
    <message>
        <location filename="ui/logwindow.cpp" line="87"/>
        <source>Text files (*.txt);;All files (*)</source>
        <translation>Text files (*.txt);;All files (*)</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="mainwindow.cpp" line="355"/>
        <source>Открыть BazaVar</source>
        <translation>Open BazaVar</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="355"/>
        <location filename="mainwindow.cpp" line="586"/>
        <source>BazaVar M2MTools (*.m2m);;Все файлы (*)</source>
        <translation>M2MTools BazaVar (*.m2m);;All files (*)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="385"/>
        <location filename="mainwindow.cpp" line="716"/>
        <source>Вариант №%1</source>
        <translation>Variant #%1</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="386"/>
        <location filename="mainwindow.cpp" line="717"/>
        <source>Вариант №%1 — Мили → Мура (в разработке)</source>
        <translation>Variant #%1 — Mealy → Moore (under development)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="403"/>
        <source>Мили → Мура — в разработке</source>
        <translation>Mealy → Moore — under development</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="405"/>
        <source>В базе нет доступных вариантов Мура → Мили.</source>
        <translation>The archive has no available Moore → Mealy variants.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="426"/>
        <location filename="mainwindow.cpp" line="721"/>
        <source>Вариант №%1 — %2</source>
        <translation>Variant #%1 — %2</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="458"/>
        <source>Архив не открыт</source>
        <translation>No archive open</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="459"/>
        <source>%1
Вариантов: %2</source>
        <translation>%1
Variants: %2</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="464"/>
        <source>Вариант №%1
Преобразование: %2</source>
        <translation>Variant No. %1
Conversion: %2</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="521"/>
        <source>Несохранённые изменения</source>
        <translation>Unsaved changes</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="522"/>
        <source>В работе есть несохранённые изменения. Сохранить их перед продолжением?</source>
        <translation>This work has unsaved changes. Save them before continuing?</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="525"/>
        <source>Если продолжить без сохранения, изменения будут потеряны.</source>
        <translation>If you continue without saving, the changes will be lost.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="526"/>
        <source>Сохранить</source>
        <translation>Save</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="527"/>
        <source>Не сохранять</source>
        <translation>Discard</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="528"/>
        <source>Отмена</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="549"/>
        <source>Сохранить учебную работу</source>
        <translation>Save assignment work</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="550"/>
        <source>variant-%1.m2mwork</source>
        <translation>variant-%1.m2mwork</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="551"/>
        <source>Работа M2MConvertor (*.m2mwork)</source>
        <translation>M2MConvertor work (*.m2mwork)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="567"/>
        <source>Открыть учебную работу</source>
        <translation>Open assignment work</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="568"/>
        <source>Работа M2MConvertor (*.m2mwork);;Все файлы (*)</source>
        <translation>M2MConvertor work (*.m2mwork);;All files (*)</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="585"/>
        <source>Укажите BazaVar для сохранённой работы</source>
        <translation>Locate the BazaVar archive for the saved work</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="639"/>
        <source>Сохранить отчёт</source>
        <translation>Save report</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="640"/>
        <source>report-variant-%1.html</source>
        <translation>report-variant-%1.html</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="641"/>
        <source>Отчёт HTML (*.html)</source>
        <translation>HTML report (*.html)</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="424"/>
        <source>Принять</source>
        <translation>Accept</translation>
    </message>
    <message>
        <location filename="ui/automatongrapheditor.cpp" line="425"/>
        <source>Отмена</source>
        <translation>Cancel</translation>
    </message>
</context>
<context>
    <name>Report</name>
    <message>
        <location filename="mainwindow.cpp" line="61"/>
        <source>%1: %2 состояний, %3 входов, %4 выходов, %5 переходов</source>
        <translation>%1: %2 states, %3 inputs, %4 outputs, %5 transitions</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="666"/>
        <source>Отчёт M2MConvertor</source>
        <translation>M2MConvertor report</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="668"/>
        <source>Преобразование эквивалентных автоматов</source>
        <translation>Conversion of equivalent automata</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="669"/>
        <source>Вариант:</source>
        <translation>Variant:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="670"/>
        <source>Направление:</source>
        <translation>Direction:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="671"/>
        <source>Дата:</source>
        <translation>Date:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="672"/>
        <source>Участники</source>
        <translation>Participants</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="673"/>
        <source>Преподаватель:</source>
        <translation>Teacher:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="674"/>
        <source>Студент:</source>
        <translation>Student:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="675"/>
        <source>Группа:</source>
        <translation>Group:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="676"/>
        <source>Зачётная книжка:</source>
        <translation>Record book:</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="677"/>
        <source>Результат</source>
        <translation>Result</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="680"/>
        <source>Таблица, матрица и граф обоих автоматов приняты. Реакция каждого автомата из a0 совпадает с реакцией варианта. Проверена заданная последовательность входов; эквивалентность на всех возможных последовательностях этой проверкой не устанавливается.</source>
        <translation>The table, matrix and graph of both automata have been accepted. Each automaton reacts from a0 as specified by the variant. The given input sequence was checked; this does not establish equivalence for all possible sequences.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="681"/>
        <source>Задание</source>
        <translation>Task</translation>
    </message>
</context>
<context>
    <name>Result</name>
    <message>
        <location filename="mainwindow.cpp" line="95"/>
        <location filename="tests/gui_smoke.cpp" line="585"/>
        <source>Приложение запущено.</source>
        <translation>Application started.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="367"/>
        <location filename="tests/gui_smoke.cpp" line="586"/>
        <source>В BazaVar нет вариантов.</source>
        <translation>BazaVar contains no variants.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="415"/>
        <source>Начата работа с вариантом №%1.</source>
        <translation>Started work on variant #%1.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="478"/>
        <source>Возврат к выбору варианта.</source>
        <translation>Returned to variant selection.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="595"/>
        <source>В текущем BazaVar нет варианта №%1.</source>
        <translation>The current BazaVar has no variant #%1.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="599"/>
        <source>Открытие работ Мили → Мура пока недоступно.</source>
        <translation>Opening Mealy → Moore work is not available yet.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="633"/>
        <source>Заполните все поля отчёта.</source>
        <translation>Complete all report fields.</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="648"/>
        <source>Не удалось сохранить отчёт: %1</source>
        <translation>Could not save the report: %1</translation>
    </message>
    <message>
        <location filename="mainwindow.cpp" line="651"/>
        <source>Отчёт создан: %1</source>
        <translation>Report created: %1</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="55"/>
        <location filename="domain/reactionchecker.cpp" line="26"/>
        <location filename="ui/automatoneditor.cpp" line="449"/>
        <location filename="ui/automatoneditor.cpp" line="949"/>
        <source>Мили</source>
        <translation>Mealy</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="56"/>
        <location filename="domain/reactionchecker.cpp" line="26"/>
        <location filename="ui/automatoneditor.cpp" line="449"/>
        <location filename="ui/automatoneditor.cpp" line="949"/>
        <source>Мура</source>
        <translation>Moore</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="68"/>
        <source>Список состояний пуст.</source>
        <translation>The state list is empty.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="69"/>
        <source>Список входных сигналов пуст.</source>
        <translation>The input signal list is empty.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="70"/>
        <source>Список выходных сигналов пуст.</source>
        <translation>The output signal list is empty.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="72"/>
        <source>Имена должны быть непустыми и уникальными внутри списка.</source>
        <translation>Names must be nonempty and unique within each list.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="74"/>
        <source>Начальное состояние не входит в список состояний.</source>
        <translation>The initial state is not in the state list.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="76"/>
        <source>Не заданы выходы всех состояний автомата Мура.</source>
        <translation>Outputs are not specified for every Moore state.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="83"/>
        <source>Переход содержит неизвестное состояние или вход.</source>
        <translation>A transition contains an unknown state or input.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="87"/>
        <source>Для состояния и входа задано несколько переходов.</source>
        <translation>Multiple transitions are defined for one state and input.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="91"/>
        <source>Переход содержит неизвестный выход.</source>
        <translation>A transition contains an unknown output.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="94"/>
        <source>Выход автомата Мура должен относиться к состоянию.</source>
        <translation>A Moore output must belong to a state.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="99"/>
        <source>Состояние содержит неизвестный выход.</source>
        <translation>A state contains an unknown output.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="143"/>
        <source>Неизвестный тип автомата.</source>
        <translation>Unknown automaton type.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="149"/>
        <source>Повреждены списки имён автомата.</source>
        <translation>The automaton name lists are corrupted.</translation>
    </message>
    <message>
        <location filename="domain/automaton.cpp" line="155"/>
        <source>Повреждён список переходов.</source>
        <translation>The transition list is corrupted.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="15"/>
        <source>Преобразование Мили → Мура находится в разработке.</source>
        <translation>Mealy → Moore conversion is under development.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="21"/>
        <source>Нужны состояния a0…a%1; начальное состояние — a0.</source>
        <translation>States a0…a%1 are required; the initial state is a0.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="24"/>
        <source>Автомат %1: требуется %2 входных и %3 выходных сигналов; введено %4 и %5.</source>
        <translation>The %1 automaton requires %2 input and %3 output signals; %4 and %5 were entered.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="35"/>
        <source>Входной сигнал должен иметь вид x0…x9.</source>
        <translation>An input signal must have the form x0…x9.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="38"/>
        <source>Выходной сигнал должен иметь вид y0…y9.</source>
        <translation>An output signal must have the form y0…y9.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="40"/>
        <source>В варианте отсутствует реакция для проверки.</source>
        <translation>The variant has no reaction to check.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="55"/>
        <location filename="tests/gui_smoke.cpp" line="584"/>
        <source>Такт %1: не задан вход %2.</source>
        <translation>Step %1: input %2 is not defined.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="59"/>
        <source>Такт %1: из %2 не задан переход по %3.</source>
        <translation>Step %1: no transition from %2 on %3.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="74"/>
        <source>Такт %1, %2 из %3: ожидались выходы %4, получены %5.</source>
        <translation>Step %1, %2 from %3: expected outputs %4, received %5.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="85"/>
        <source>Такт %1: из %2 нет перехода по %3 для продолжения реакции.</source>
        <translation>Step %1: no transition from %2 on %3 to continue the reaction.</translation>
    </message>
    <message>
        <location filename="domain/reactionchecker.cpp" line="92"/>
        <source>Верно. Реакция из a0 совпадает с вариантом во всех %1 тактах.</source>
        <translation>Correct. The reaction from a0 matches the variant. Steps checked: %1.</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="5"/>
        <source>Информация</source>
        <translation>Info</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="7"/>
        <source>Предупреждение</source>
        <translation>Warning</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="9"/>
        <source>Ошибка</source>
        <translation>Error</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="16"/>
        <source>Приложение</source>
        <translation>Application</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="18"/>
        <source>Архив</source>
        <translation>Archive</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="20"/>
        <source>Работа</source>
        <translation>Work</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="22"/>
        <source>Проверка</source>
        <translation>Validation</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="24"/>
        <source>Отчёт</source>
        <translation>Report</translation>
    </message>
    <message>
        <location filename="domain/result.cpp" line="26"/>
        <source>Перевод</source>
        <translation>Translation</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="20"/>
        <location filename="infrastructure/progressstore.cpp" line="27"/>
        <location filename="infrastructure/progressstore.cpp" line="59"/>
        <source>Ошибка файла: %1.</source>
        <translation>File error: %1.</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="34"/>
        <source>Работа сохранена: %1.</source>
        <translation>Work saved: %1.</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="38"/>
        <location filename="infrastructure/progressstore.cpp" line="90"/>
        <source>Не удалось прочитать работу.</source>
        <translation>Could not read the work file.</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="67"/>
        <source>Файл прогресса повреждён.</source>
        <translation>The progress file is corrupted.</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="77"/>
        <source>Версия файла прогресса не поддерживается. Работы старого формата с полными автоматами нужно начать заново.</source>
        <translation>This progress file version is not supported. Work based on the old full-automaton format must be started again.</translation>
    </message>
    <message>
        <location filename="infrastructure/progressstore.cpp" line="86"/>
        <source>Работа открыта: %1.</source>
        <translation>Work opened: %1.</translation>
    </message>
    <message>
        <location filename="infrastructure/translationmanager.cpp" line="13"/>
        <source>Язык %1 не поддерживается.</source>
        <translation>Language %1 is not supported.</translation>
    </message>
    <message>
        <location filename="infrastructure/translationmanager.cpp" line="20"/>
        <source>Не удалось загрузить перевод %1.</source>
        <translation>Could not load the %1 translation.</translation>
    </message>
    <message>
        <location filename="infrastructure/translationmanager.cpp" line="36"/>
        <source>Язык интерфейса изменён: %1.</source>
        <translation>Interface language changed: %1.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="106"/>
        <source>Архив содержит неверный номер или тип варианта.</source>
        <translation>The archive contains an invalid variant number or type.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="121"/>
        <source>В варианте повреждены количества состояний или сигналов.</source>
        <translation>A variant contains invalid state or signal counts.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="125"/>
        <source>В варианте отсутствует реакция автомата.</source>
        <translation>The variant has no automaton reaction.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="135"/>
        <source>Повреждён такт реакции автомата.</source>
        <translation>An automaton reaction step is corrupted.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="139"/>
        <source>Неверные или повторяющиеся выходы в такте реакции.</source>
        <translation>Invalid or duplicate outputs in a reaction step.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="150"/>
        <source>В варианте №%1 количества сигналов исходного автомата не совпадают с реакцией. Исправьте вариант в M2MTools.</source>
        <translation>The source signal counts in variant #%1 do not match its reaction. Correct the variant in M2MTools.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="159"/>
        <source>Изображение задания превышает 10 МБ.</source>
        <translation>The task image exceeds 10 MB.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="165"/>
        <source>Изображение задания повреждено.</source>
        <translation>The task image is corrupted.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="176"/>
        <source>Мили → Мура</source>
        <translation>Mealy → Moore</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="177"/>
        <source>Мура → Мили</source>
        <translation>Moore → Mealy</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="189"/>
        <source>Не удалось открыть BazaVar: %1</source>
        <translation>Could not open BazaVar: %1</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="197"/>
        <source>Нужен новый архив .m2m с реакциями из M2MTools (ветка Lite). Старые базы с автоматами не поддерживаются.</source>
        <translation>A new .m2m reaction archive from M2MTools (Lite branch) is required. Old full-automaton archives are not supported.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="212"/>
        <source>BazaVar повреждён либо создан несовместимой версией M2MTools.</source>
        <translation>BazaVar is corrupted or was created by an incompatible version of M2MTools.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="221"/>
        <source>Расшифрованные данные BazaVar повреждены.</source>
        <translation>The decrypted BazaVar data is corrupted.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="229"/>
        <source>Версия BazaVar не поддерживается.</source>
        <translation>This BazaVar version is not supported.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="238"/>
        <source>Архив содержит повреждённый вариант.</source>
        <translation>The archive contains a corrupted variant.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="247"/>
        <source>В BazaVar повторяется вариант №%1.</source>
        <translation>BazaVar contains duplicate variant #%1.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="261"/>
        <source>Загружено вариантов: %1.</source>
        <translation>Variants loaded: %1.</translation>
    </message>
    <message>
        <location filename="infrastructure/variantarchive.cpp" line="264"/>
        <source>Не удалось прочитать архив.</source>
        <translation>Could not read the archive.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="450"/>
        <source>Количество состояний или сигналов автомата %1 не соответствует варианту.</source>
        <translation>The number of states or signals in the %1 automaton does not match the variant.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="456"/>
        <source>Размеры таблицы приняты.</source>
        <translation>Table dimensions accepted.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="463"/>
        <source>Таблица очищена.</source>
        <translation>Table cleared.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="498"/>
        <source>Заполните входные сигналы слева: разные x0…x9.</source>
        <translation>Enter distinct input signals x0…x9 in the left column.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="510"/>
        <source>Выходы состояний: введите y0…y9 через запятую, без повторов, либо «-».</source>
        <translation>State outputs: enter y0…y9 separated by commas, without duplicates, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="524"/>
        <source>Для перехода Мили укажите «состояние / выход». </source>
        <translation>For a Mealy transition, enter “state / output”. </translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="531"/>
        <source>Без следующего состояния выходы не задаются. Для отсутствия перехода используйте -/-.</source>
        <translation>Outputs require a destination state. Use -/- for no transition.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="537"/>
        <source>Переход ссылается на неизвестное состояние «%1».</source>
        <translation>A transition refers to unknown state “%1”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="545"/>
        <source>Выходы перехода: введите y0…y9 через запятую, без повторов, либо «-».</source>
        <translation>Transition outputs: enter y0…y9 separated by commas, without duplicates, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="630"/>
        <source>Введите размерность матрицы одним положительным целым числом.</source>
        <translation>Enter the matrix dimension as a single positive integer.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="635"/>
        <source>Размерность матрицы должна совпадать с числом состояний принятой таблицы: %1.</source>
        <translation>The matrix dimension must match the number of states in the accepted table: %1.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="640"/>
        <source>Матрица слишком велика для редактора (более 250 000 ячеек).</source>
        <translation>The matrix is too large for the editor (over 250,000 cells).</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="724"/>
        <source>Сначала создайте матрицу правильной размерности.</source>
        <translation>Create a matrix with the correct dimensions first.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="733"/>
        <source>Выходы состояния %1: нужны y0…y9 через запятую или «-».</source>
        <translation>Outputs of state %1: enter y0…y9 separated by commas, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="751"/>
        <source>Метка дуги Мили должна иметь вид «вход / выход».</source>
        <translation>A Mealy edge label must have the form “input / output”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="759"/>
        <source>Матрица содержит неизвестный вход «%1».</source>
        <translation>The matrix contains unknown input “%1”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="764"/>
        <source>Для одной пары состояние-вход задано несколько дуг.</source>
        <translation>Multiple edges are defined for one state/input pair.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="773"/>
        <source>Выходы в матрице: нужны y0…y9 через запятую, без повторов, либо «-».</source>
        <translation>Matrix outputs: enter y0…y9 separated by commas, without duplicates, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="827"/>
        <source>Вершина автомата Мура должна иметь подпись «состояние / выходы».</source>
        <translation>A Moore vertex must have the label “state / outputs”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="835"/>
        <source>Вершина содержит неизвестное состояние «%1».</source>
        <translation>A vertex contains unknown state “%1”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="839"/>
        <source>Состояние «%1» представлено несколькими вершинами.</source>
        <translation>State “%1” is represented by multiple vertices.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="848"/>
        <source>Выходы вершины «%1»: нужны y0…y9 через запятую, без повторов, либо «-».</source>
        <translation>Outputs of vertex “%1”: enter y0…y9 separated by commas, without duplicates, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="854"/>
        <source>На графе представлены не все состояния автомата.</source>
        <translation>The graph does not represent all states.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="861"/>
        <source>Дуга ссылается на неизвестную вершину.</source>
        <translation>An edge refers to an unknown vertex.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="867"/>
        <source>У каждой дуги должна быть подпись.</source>
        <translation>Every edge must have a label.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="876"/>
        <source>Дуга Мили должна иметь подпись «вход / выходы».</source>
        <translation>A Mealy edge must have the label “input / outputs”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="882"/>
        <source>На дуге автомата Мура указывается только вход.</source>
        <translation>A Moore edge must contain only input labels.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="887"/>
        <source>Дуга содержит неизвестный вход «%1».</source>
        <translation>An edge contains unknown input “%1”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="896"/>
        <source>Выходы дуги: нужны y0…y9 через запятую, без повторов, либо «-».</source>
        <translation>Edge outputs: enter y0…y9 separated by commas, without duplicates, or “-”.</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="950"/>
        <source>Таблица</source>
        <translation>Table</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="950"/>
        <source>Матрица</source>
        <translation>Matrix</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="950"/>
        <source>Граф</source>
        <translation>Graph</translation>
    </message>
    <message>
        <location filename="ui/automatoneditor.cpp" line="951"/>
        <source>%1 — %2</source>
        <translation>%1 — %2</translation>
    </message>
    <message>
        <location filename="ui/logwindow.cpp" line="80"/>
        <source>Не удалось сохранить журнал: %1.</source>
        <translation>Could not save the log: %1.</translation>
    </message>
    <message>
        <location filename="ui/logwindow.cpp" line="83"/>
        <source>Журнал сохранён: %1.</source>
        <translation>Log saved: %1.</translation>
    </message>
</context>
<context>
    <name>TaskImage</name>
    <message>
        <location filename="ui/widgets/taskimage.h" line="41"/>
        <source>Изображение задания отсутствует</source>
        <translation>The task image is missing</translation>
    </message>
</context>
</TS>
