# Computer-Graphics-spring-2021
Мои реализации лабораторных работ<br>

# Лабораторная работа 1: Изучение простых преобразований изображений

Поддерживаются pgm (P5) и ppm (P6) изображения.<br>

Аргументы:<br>
<b>lab1.exe <имя_входного_файла> <имя_выходного_файла> <преобразование>,</b><br>
где <преобразование>:<br>
<ul>
  <li>0 - инверсия;</li>
  <li>1 - зеркальное отражение по горизонтали;</li>
  <li>2 - зеркальное отражение по вертикали;</li>
  <li>3 - поворот на 90 градусов по часовой стрелке;</li>
  <li>4 - поворот на 90 градусов против часовой стрелки.</li>
</ul>
<br>
  
# Лабораторная работа 2: Изучение цветовых пространств

Поддерживаются pgm (P5) и ppm (P6) изображения.<br>

Аргументы:<br>
<b>lab2.exe -f <начальное_цветовое_пространство> -t <итоговое_цветовое_пространство> -i <количество_входных_файлов> <имя_входного_файла> -o <количество_выходных_файлов> <имя_выходного_файла>,</b><br>
где:<br>
<ul>
  <li><цветовое_пространство> - RGB / HSL / HSV / YCbCr.601 / YCbCr.709 / YCoCg / CMY;</li>
  <li><количество_файлов> - 1 или 3;</li>
  <li>
    <имя_файла>:<br>
      для <количество_файлов>=1 - просто имя файла в формате ppm;<br>
      для <количество_файлов>=3 - шаблон имени вида <b>name.pgm</b>, что соответствует файлам <b>name_1.pgm</b>, <b>name_2.pgm</b> и <b>name_3.pgm</b> для каждого канала соответственно.
  </li>
</ul>

Поддерживается произвольный порядок аргументов (-f, -t, -i, -o).<br>
Везде 8-битные данные и полный диапазон (0..255, PC range).<br>

# Лабораторная работа 3: Изучение алгоритмов псевдотонирования изображений

Поддерживаются только P5 изображения.<br>
Учитывается гамма-коррекция.<br>

Аргументы:<br>
<b>lab3.exe <имя_входного_файла> <имя_выходного_файла> <градиент> <дизеринг> <битность> <гамма>,</b><br>
где:<br>
<ul>
  <li><имя_входного_файла>, <имя_выходного_файла>: формат файлов: PGM P5; ширина и высота берутся из <имя_входного_файла>;</li>
  <li><градиент>: 0 - используем входную картинку, 1 - рисуем горизонтальный градиент (0-255);</li>
  <li>
    <дизеринг> - алгоритм дизеринга:
      <ul>
        <li>0 – Нет дизеринга;</li>
        <li>1 – Ordered (8x8);</li>
        <li>2 – Random;</li>
        <li>3 – Floyd–Steinberg;</li>
        <li>4 – Jarvis, Judice, Ninke;</li>
        <li>5 - Sierra (Sierra-3);</li>
        <li>6 - Atkinson;</li>
        <li>7 - Halftone (4x4, orthogonal);</li>
      </ul>
  </li>
  <li><битность> - битность результата дизеринга (1..8);</li>
   <li><гамма>: 0 - sRGB гамма, иначе - обычная гамма с указанным значением.</li>
</ul>

# Лабораторная работа 4: Изучение алгоритмов масштабирования изображений

<u>Комментарий</u>: Выполнено только <b>частичное</b> решение.<br>

Аргументы: <br>
lab4.exe <имя_входного_файла> <имя_выходного_файла> <результирующая_ширина> <результирующая_высота> <d_x> <d_y> <гамма> <способ_масштабирования> \[<В> <С>\],<br>
где:<br>
<ul>
  <li><имя_входного_файла>, <имя_выходного_файла> - имена входного и выходного файлов в формате PNM P5 или P6;</li>
  <li><результирующая_ширина>, <результирующая_высота> - ширина и высота результирующего изображения, натуральные числа;</li>
  <li>d_x и d_y - смещение центра результата относительно центра исходного изображения, вещественные числа в единицах результирующего изображения;</li>
  <li><гамма> - коэффициент гамма-коррекции, вещественное число (0 = sRGB);</li>
  <li><способ масштабирования>:
    <ul>
      <li>0 – ближайшая точка (метод ближайшего соседа);</li>
      <li>1 – билинейная интерполяция;</li>
      <li>2 – Lanczos3;</li>
      <li>3 – BC-сплайны. Для этого способа могут быть указаны ещё два параметра: B и C, по умолчанию 0 и 0.5 (Catmull-Rom).</li>
    </ul>
  </li>
</ul>
Информация про BC-сплайны: https://en.wikipedia.org/wiki/Mitchell%E2%80%93Netravali_filters <br>

В <b>частичном</b> решении:
<ul>
  <li>работа только с чёрно-белыми изображениями формата P5;</li>
  <li>только увеличение изображения;</li>
  <li>не учитываются gamma (всегда = 1), d_x и d_y (левый верхний угол исходного изображения совпадает с итоговым).</li>
</ul>  
