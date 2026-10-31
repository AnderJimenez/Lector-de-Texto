# Lector de texto en español

Procesador de consola que analiza oraciones en español y las devuelve con
colores según una clasificación léxica sencilla. La salida distingue sujeto,
preposiciones, nexos, verbos, verbos en pasado, artículos, pronombres y
adverbios. La opción `--arbol` muestra además un árbol semántico simplificado
en el orden de la oración.

También evalúa expresiones de lógica proposicional escritas con `AND`, `OR`,
`->` y `<->`. Acepta tanto `A AND B` como la notación `A(AND)B`, y muestra
su tabla de verdad:

- `A AND B`: ambos eventos ocurren.
- `A OR B`: al menos uno de los eventos ocurre.
- `A -> B`: la implicación solo es falsa cuando A es verdadera y B es falsa.
- `A <-> B`: ambos sucesos tienen el mismo valor de verdad.

Las fórmulas deben escribirse en una línea independiente usando proposiciones
simples (por ejemplo, `A`, `B`, `llueve`, `estudio`) y los operadores
anteriores. Se aceptan operadores escritos en mayúsculas o minúsculas,
paréntesis y espacios:

```text
A AND B
a and b
(A OR B) -> C
A(AND)B
```

También se pueden usar conectores en español:

```text
llueve y hace frío
llueve o hace calor
llueve entonces llevo paraguas
llueve por lo tanto llevo paraguas
llueve dado que el suelo está mojado
llueve ya que el suelo está mojado
```

El programa interpreta `y` como `AND`, `o` como `OR`, `entonces` y
`por lo tanto` como `->`, y `dado que` y `ya que` como `<->`, según las
reglas solicitadas. `ni` se interpreta como una relación de negación conjunta:
solo es verdadera cuando ambas proposiciones son falsas. Las proposiciones
pueden contener varias palabras.

## Compilar

Con un compilador compatible con C++17:

```text
g++ -std=c++17 -Wall -Wextra -pedantic Base.cpp -o lector
```

En Windows también puede usarse:

```text
g++ -std=c++17 -Wall -Wextra -pedantic Base.cpp -o lector.exe
```

## Usar

```text
lector
lector --arbol
lector --sin-color
```

Escriba una oración y presione Enter. El programa conserva la puntuación y
procesa cada línea de forma independiente. La clasificación es heurística:
reconoce un vocabulario base y terminaciones verbales frecuentes, por lo que
puede ampliarse en `Base.cpp` para cubrir vocabulario especializado.

Las expresiones lógicas se detectan automáticamente cuando contienen alguno de
los operadores anteriores.

El archivo `Frases` contiene ejemplos agrupados por relación. Las líneas que
comienzan con `#` son encabezados y se ignoran al ejecutar el archivo:

```powershell
Get-Content -Encoding UTF8 .\Frases | .\lector.exe --sin-color
```

## Base de datos de palabras

El programa mantiene las palabras personalizadas en `palabras.txt`, en el
mismo directorio desde donde se ejecuta. Se cargan al iniciar y se guardan
automáticamente con estos comandos:

```text
/agregar verbo caminar
/agregar adverbio rápidamente
/agregar sujeto lucía
```

Después de agregar una palabra, las siguientes oraciones de la misma ejecución
la reconocerán con la categoría correspondiente. El archivo usa el formato
`tipo|palabra`, una entrada por línea. Puede consultar la ayuda desde el
programa con `/ayuda`.