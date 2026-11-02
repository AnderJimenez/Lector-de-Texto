# Revisor de reglas proposicionales

Programa de consola que recibe una base de conocimiento escrita en español,
con una regla o hecho por línea. Identifica las proposiciones simples,
conserva el mismo átomo cuando el significado se repite y genera un tableau
semántico para comprobar si todas las reglas pueden ser verdaderas a la vez.

## Conectivos reconocidos

| Texto | Símbolo |
| --- | --- |
| y, e | ∧ |
| o, u | ∨ |
| no | ¬ |
| si... entonces | → |
| si y solo si | ↔ |

También se aceptan `AND`, `OR`, `NOT`, `->` y `<->`. Las proposiciones pueden
tener varias palabras, por ejemplo `es de día` o `voy a trabajar`.
`ni` se interpreta como una relación verdadera únicamente cuando ambas
proposiciones son falsas.

## Ejemplo

Entrada:

```text
Si es de día, entonces voy a trabajar
Si llueve, entonces no voy a trabajar
Es de día
```

La salida muestra:

1. La tabla de átomos, por ejemplo `p = es de día` y `q = voy a trabajar`.
2. Cada regla convertida a una fórmula (`R1: p → q`).
3. Las ramas del tableau, con las fórmulas aplicadas.
4. Las ramas cerradas con `✕` cuando contienen un átomo y su negación.
5. Las ramas abiertas como modelos (`p = V, q = V, r = F`).
6. La conclusión de satisfacibilidad de la base de conocimiento.

El tableau aplica las expansiones de `∧`, `¬∨`, `¬→`, `¬¬`, `∨`, `→`,
`↔`, `NI`, `¬∧` y `¬↔` mediante las reglas equivalentes para fórmulas con signo
verdadero o falso.

## Compilar

Con un compilador compatible con C++17:

```text
g++ -std=c++17 -Wall -Wextra -pedantic Base.cpp -o lector
```

En Windows:

```text
g++ -std=c++17 -Wall -Wextra -pedantic Base.cpp -o lector.exe
```

## Usar

El modo predeterminado revisa cada frase inmediatamente. Escriba una regla en
una línea y recibirá su tableau antes de introducir la siguiente; ya no es
necesario esperar hasta `Ctrl+Z`:

```powershell
Get-Content -Encoding UTF8 .\reglas.txt | .\lector.exe --sin-color
```

Las frases sin un conector reconocido se anuncian inmediatamente como no
detectadas. Los tres documentos de referencia se encuentran en [Reglas](./Reglas);
la implementación aplica sus reglas de descomposición lineales, bifurcaciones,
cierre de ramas por contradicción y modelos de ramas abiertas.

Para consultar la ayuda:

```text
lector --ayuda
```

El modo anterior de clasificación léxica se conserva con `--texto`:

```text
lector --texto --sin-color
```

## Ventana gráfica

La interfaz gráfica se inicia desde [Base.cpp](./Base.cpp), que incluye
[Ventana.cpp](./Ventana.cpp) como implementación secundaria. En Windows,
compílala con MinGW:

```powershell
g++ -std=c++17 -mwindows -Wall -Wextra -pedantic Base.cpp -o lector.exe
.\lector.exe
```

Escribe una frase como `llueve y hace frío`, pulsa **Analizar** y la ventana
mostrará la relación y su tabla de verdad. También acepta `o`, `entonces`,
`dado que`, `ya que`, `ni`, `AND`, `OR`, `->` y `<->`. La interfaz usa Unicode
de Windows, por lo que acepta frases con acentos como `llueve y está frío`.

El archivo [Frases.txt](./Frases.txt) contiene ejemplos de oraciones y las
líneas que empiezan con `#` se ignoran. Para procesarlo:

```powershell
Get-Content -Encoding UTF8 .\Frases.txt | .\lector.exe --sin-color
```

La implementación principal del árbol está en
[ArbolSemantico.cpp](./ArbolSemantico.cpp). `Base.cpp` es un punto de entrada
compatible que incluye ese archivo, por lo que ambos comandos siguientes
generan el mismo programa:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic Base.cpp -o lector.exe
g++ -std=c++17 -Wall -Wextra -pedantic ArbolSemantico.cpp -o arbol.exe
```

Si una línea no contiene un conector lógico reconocido, el programa informa
que no detectó una relación en vez de tratarla silenciosamente como una regla.
