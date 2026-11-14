# DSL unificado (diagram + wireframe)

Un solo lenguaje con dos usos: figuras (`.dg`) y pantallas (`.wf`). Comparten sintaxis, vocabulario de atributos, `def`, `data`, `repeat`, `if`, `import` y flechas. Este documento es la referencia; los casos de `cases/` son el contrato entre C++, JS y Python.

Estado: implementado en C++ (`*_tec.hpp`) y verificado contra los SVG de referencia de `dg/` (148 de 149 idénticos). Los puertos a JS y Python deben reproducir lo que dice este documento.

## 1. Sintaxis (módulo `dsl`)

- Una línea es una lista de ítems separados por `;`. La sangría (espacios, nunca tabuladores) anida.
- Un ítem es `nombre palabra "cadena" clave=valor ...`. Las palabras sueltas son banderas (`bold`, `light`, `elbow`).
- `#` abre un comentario hasta fin de línea. Las cadenas llevan `\"`, `\\` y `\n`.
- Variables `$nombre` en cualquier valor; `$id` es el id del uso de un `def`.
- Todo el resto (`def`, `data`, `repeat n= var=`, `if`, `import`, `content`) es como en `dsl_tec.hpp`.

## 2. Estructura de un archivo

```
import shapes
diagram fig13-1 frame style=oreilly size=11 caption="Figure 13-1. Sharding in Redis"
  row gap=130
    ...
screen cart "Carrito" w=340
  ...
```

- `import` va primero. Después, uno o más bloques `diagram` o `screen`; cada bloque produce un SVG.
- Desaparecen las directivas `@frame`, `@style`, `@size`, `@caption`: son opciones del encabezado (`frame`, `style=`, `size=`, `caption=`).
- El id puede omitirse; entonces es el nombre del archivo.

## 3. Vocabulario común

| Atributo | Significado | Reemplaza |
|---|---|---|
| `w`, `h` | ancho y alto. En `box`, `text` y `shape` son mínimos: el elemento crece hasta su contenido. En `canvas` y en `box` con hijos son exactos | `width`, `height` de wireframe |
| `gap`, `pad` | separación entre hijos / margen interior. `pad` escrito en el elemento gana sobre `padx`/`pady` del estilo. `grid` admite `gapx` y `gapy` | igual |
| `fill`, `sw`, `dash` | relleno, grosor, trazo | igual |
| `caption` | texto bajo o sobre la figura | igual |
| `at=x,y` | posición absoluta dentro de un `canvas` | igual |
| `minw`, `minh` | tamaño mínimo | nuevo |
| `alignx=ID`, `aligny=ID` | centrar sobre otro elemento de un `canvas` (§5) | `below`, `right`... |
| `col=`, `row=`, `span=` | celda de un `grid` | igual (wf) |

Los colores son nombres del tema (`light`, `blue`, `primary-soft`, ...) o `#rrggbb`.

## 3b. Estilos y fuentes

Los estilos son datos, no código: un bloque `style` en una librería da valores por defecto por tipo de elemento.

```
style oreilly
  box radius=8 sw=1.6 padx=14 pady=8
  arrow sw=2.5

style silberschatz
  box radius=0 sw=1.3 padx=8 pady=4
  arrow sw=1.3
```

- `style=oreilly` en el encabezado lo elige; se pueden apilar (`style=silberschatz,local`) y cada capa pisa a la anterior. Prioridad: atributo escrito en el elemento, luego el estilo, luego el valor interno.
- Un componente compuesto (`def`) puede fijar sus propios valores y así sobreescribir el estilo solo en su cuerpo.
- Fuentes: sans (Helvetica) por defecto o la bandera `mono`; no hay atributo `font=`. Para medir texto se usa la tabla de Helvetica multiplicada por un factor fijo (1.1 en `mono`, 1.07 en negrita); la letra real no se mide.

## 3c. Vocabulario por niveles

El núcleo valida todos sus atributos (uno desconocido da error con sugerencia). Están separados en tres niveles; un diagrama normal solo necesita el primero.

| Nivel | Atributos |
|---|---|
| **Núcleo** (27) | `id` `w` `h` `minw` `minh` · `gap` `pad` `padx` `pady` `align` · `cols` `span` `at` `alignx` `aligny` `x` `y` · `fill` `stroke` `sw` `dash` `radius` · `size` `color` `caption` `title` · `style` |
| **Formas** (13, dentro de `shape`) | `r` `cx` `cy` `rx` `ry` `x1` `y1` `x2` `y2` `d` `pts` `tx` `ty` |
| **Extensiones** (24) | Flechas y notas: `tip` `tail` `label` `side` `margin` · Trazo: `alpha` `linecap` `join` · Texto: `base` `rotate` `capsize` `captionpos` `captionstyle` · Reparto: `gapx` `gapy` `rspan` `valign` `weights` `layout` `around` `padtop` · Otros: `border` `theme` `metric` |

Banderas: `bold` `italic` `mono` (texto) · `left` `right` `center` `top` `bottom` · `wrap` · `stretch` `grow` · `float` · `frame` · `open` · `elbow` `both` `curve` `full`. En C++ y JS la lista está en `ATTRS_CORE`, `ATTRS_SHAPE` y `ATTRS_EXT` (`figure_tec`); `ATTRS` es su unión.

## 4. Elementos

Contenedores (calculan la posición de sus hijos): `row`, `col`, `grid cols=N`, `group "Título"`, `layer` (superpone sus hijos), `canvas w= h=` (hijos con `at=`, `alignx=` o `aligny=`), y `box` con hijos (se apila como una `col`).

Hojas: `box`, `text`, `spacer`, `shape` (forma propia, §6).

`align` vale `start`, `center`, `end` o `stretch`. Reglas:
- Sin `align`, una `col` estira sus `box` al ancho de la columna y centra el resto; un `text` en una `col` se centra salvo que lleve `left` o `right`.
- Con `align` explícito, el contenido también se reparte en el eje principal cuando el contenedor tiene tamaño fijo (`box h=62 align=center` centra verticalmente sus hijos).
- En un `grid`, `align` se aplica a la celda en horizontal y en vertical.
- Un `box` sin texto mide al menos 36×36; `text` con bandera de color (`text white`) dibuja su fondo.

Componentes de wireframe (`button`, `field`, `select`, `divider`, `card`, `title`, `steps`, ...): son `def` de la librería `ui`, escritos con `layer`, `shape`, `text`, `row`, `col`. Un wireframe es un diagrama que importa `ui`. `lines n` y `table` quedan como primitivos hasta que existan contador y aritmética en los `def`.

## 5. Posición

Dos formas:

```
box a "A" at=0,90 w=110 h=44        # absoluta, dentro de un canvas
box b "B" alignx=a at=0,10          # centrada horizontalmente sobre a (aligny= en vertical)
```

- Se evalúan en el orden del archivo: solo se puede referir a elementos ya declarados (el error lo dice y sugiere el nombre más parecido).
- Dentro de `row`, `col` y `grid` no se usa posición; mandan el contenedor y su `gap`.
- Se eliminaron `below=`, `above=`, `right=`, `left=`, `dx=`, `dy=`: en el corpus casi todo es `at=` o contenedores.
- El tamaño del SVG es la unión del lienzo declarado, los elementos (salvo los `float`), las flechas (±2 px) y las etiquetas de las flechas, más el margen (6 por defecto).

Regla práctica: si la figura tiene estructura (filas, columnas, grupos), usar contenedores. `at=` queda para dibujos libres.

## 6. Formas y expresiones

El cuerpo de un `shape` se dibuja con `rect`, `ellipse`, `path`, `poly`, `line` y `label`; `set` define variables. Sus valores numéricos son expresiones con `w` y `h` de la forma:

```
shape $id "$text" w=$w h=$h
  rect fill=self sw=1.6
  line x1=10 x2=w-10 y1=h/2 y2=h/2 dash="0 5"
```

- Gramática: números, `w`, `h`, variables de `set`, `+ - * /`, paréntesis, `min`, `max`, `abs`, `sin`, `cos` y la constante `pi`. Sin condicionales.
- `set nombre=expr` define una variable local del `shape`. `repeat n=N var=k` expone `$k` (0..N-1).
- En `path d=` y `poly pts=`, las letras mayúsculas son comandos (`M L H V C S Q T A Z`, solo absolutos) y cada valor es una expresión sin espacios fuera de paréntesis; las variables empiezan con minúscula.
- Se evalúan al dibujar, no al expandir: `w` y `h` salen del layout.
- Los porcentajes (`12%`) valen en `layer` (`x=38% w=62%`) y en las anclas (`t.bottom@12%`).

## 7. Flechas

```
a -> b                           # entre ids
a -- b                           # sin punta
a.bottom -> b.top elbow          # con anclas: top bottom left right
pub -> t1,t2 sw=3                # varios destinos
a -> b -> c                      # cadena
app~2.right -> other             # el segundo hijo de app
a.bottom@25% -> b.top@=c         # ancla al 25 % del lado, o a la altura de c
a -> b label="Escribe" side=right
a -> b both                      # punta en los dos extremos
a -> b dash="7 5"                # discontinua
```

- Opciones: `label`, `side`, `sw` (2.5 por defecto), `color`, `dash`, `elbow`, `curve`, `both`, `full`, `tip`, `tail`, `margin`.
- Punta: largo `max(11, 3.3·sw)`, mitad del ancho `0.46·largo`. El trazo se acorta `0.85·largo`, salvo que el último tramo mida menos de `1.2·largo`.
- `elbow`: sale y entra con tramos de `margin` (26 por defecto). Si el destino queda detrás de la salida, rodea (en U, en Z de siete puntos o en cinco puntos para lados perpendiculares). Se conservan los vértices colineales.
- Etiqueta: en el punto medio del tramo más largo de la ruta sin acortar, en cursiva, con un halo blanco de 4 px y a 7 px del trazo. Va arriba si el tramo es horizontal; a la derecha si es vertical, o a la izquierda si el tramo es un rodeo hacia afuera. `side=` lo cambia.
- Eliminados (se expresan con lo que queda): `<-` (se escribe al revés), `<->` (`both`), `..>` (`dash`), `thick` (`sw=7`), `dashed`, `via`, `route`, `seg`, `plain`, `lstyle`, `lalign`, `ldx`, `ldy`.
- Entre hermanos, una flecha sin extremos une el elemento anterior con el siguiente:

```
row
  box "Cliente"; -> "petición"; box "Servidor"; -> "consulta"; box "BD"
```

- En wireframe, `-> pantalla` tras un `button`, `link` o `card` crea el enlace del flujo. Vale sobre cualquier uso de `def`: la flecha sale del primer elemento del cuerpo (se le pone un id si no lo tiene) y su etiqueta es el primer texto entre comillas. El destino es el id de un elemento del mismo diagrama; si no existe ahí, el enlace se ignora sin error, de modo que la pantalla suelta se dibuja igual y el enlace solo aparece cuando se incrusta en un flujo (ver abajo).
- Las flechas de un flujo no cruzan las pantallas: salen del borde de la pantalla que contiene el botón (a la altura del botón), entran por el borde de la pantalla destino y rodean las demás y el título por los espacios libres, en tramos horizontales y verticales con carriles separados. Las etiquetas buscan un sitio sin pantallas ni otras etiquetas. Las flechas se tienden de una en una y no se minimizan los cruces entre ellas.
- `->#` en lugar de `->` documenta el enlace pero el flujo no lo dibuja: `button "Cancelar" outline ->#  login`. Sirve para los enlaces de retorno (volver, cancelar, cerrar sesión).
- `diagram flujo links=forward` dibuja solo los enlaces hacia una pantalla que aparece después en el flujo (el orden de los `screen id=` es el orden de las pantallas); los que van hacia atrás se omiten. `links=none` no dibuja ninguno y `links=all` (por omisión) los dibuja todos.
- Un diagrama puede incrustar otro anterior del mismo archivo: un elemento sin contenido ni texto propio cuyo `id=` es el nombre de ese diagrama (`screen id=login`) se sustituye por el contenido del diagrama `login`. Los demás atributos del elemento (`w=`, `span=`...) se conservan. Con esto un flujo une pantallas ya definidas y los `-> destino` de sus botones se dibujan entre ellas:

```
diagram login style=ui
  screen "Entrar" w=200
    button "Entrar" -> panel
diagram panel style=ui
  screen "Panel" w=200
diagram flujo style=ui,flow
  flow "Acceso" columns=2 gapx=100
    screen id=login
    screen id=panel
```

### Anotaciones (`callout`)

Notas fuera del dibujo, unidas con una línea fina que termina en un punto sobre el elemento. Se escriben como las flechas (en cualquier parte del diagrama, no ocupan lugar en el layout):

```
callout email "El correo debe verificarse antes de entrar"
callout clave.right "Campo enmascarado"          # ancla en un lado del elemento
callout entrar "Acción principal.\nDeshabilitada hasta validar"   # varias líneas con \n
callout login.top "Saludo" side=top               # la nota va arriba
callout olvido@30% "Recuperación" side=bottom     # ancla al 30 % del lado
```

- El destino es un id (con `.lado`, `@pct` o `~n`, como en las flechas). La nota va en `side=` (`right` por defecto; si falta, el lado del destino).
- La caja se coloca a `gap` (48) del borde del dibujo, centrada en el ancla; en cada lado se ordenan por el ancla y se apilan con 8 px de separación para no solaparse. El tamaño del SVG las incluye.
- Atributos: `side`, `gap`, `size`, `padx` (12), `pady` (8), `radius` (4), `fill`, `stroke` (caja, línea y punto), `color` (texto) y las banderas `bold`, `italic`, `mono`. Colores por defecto: `callout`, `callout-border`, `callout-ink` (en `THEME`). El estilo `ui` los lleva a `size=10`.
- Ejemplo: `wf/anotaciones-login.dg`. Para poder anotar `field`, `button` y `link` de `ui`, aceptan `id=`.

### Temas CSS (`theme=`)

`diagram ... theme=balsamiq` (o `textod ENTRADA.dg -T balsamiq` para los diagramas que no lo traen) incrusta en el SVG el CSS base de colores (`c-f-*`, `c-s-*` con variables `--c-*`) y el de `balsamiq.css`, y pone la clase `theme-balsamiq` en el `<svg>`. Vale igual para wf, charts y dg.

- Varios temas se separan con coma (`theme=balsamiq,print`): sus hojas van en ese orden, la última gana, y la raíz lleva una clase por tema. `lib/temas/print.css` es la paleta en grises para imprimir; solo redefine las variables `--c-*`, así que se combina con cualquier otro tema. La paleta en color por defecto vive en `THEME::PALETTE` (valores de respaldo de `var(--c-nombre, …)`), de modo que sin tema el SVG no cambia.
- Si varios SVG con temas distintos van en línea en una misma página HTML, el orden entre sus hojas lo decide el documento: para combinarlos con garantía conviene usar `<img>` o un solo SVG por tema.
- El tema se busca como un `import`: `NOMBRE.css` junto al archivo, en los `-L` o en las librerías incluidas (`lib/temas/`). Si no existe es un error con la línea.
- Una hoja de tema es CSS normal: variables (`--c-primary`, `--c-font`…), reglas por etiqueta (`rect`, `text`, `line`, `path`, `polygon`, `ellipse`) y filtros. Todas las reglas cuelgan de `.theme-NOMBRE` para no afectar al resto de la página cuando el SVG va en línea.
- No hay ganchos por componente: los `def` acaban en esas primitivas y en las clases de color, así que un componente nuevo ya queda cubierto.
- Ejemplo: `lib/temas/balsamiq.css` (letra Patrick Hand, licencia OFL, incrustada en base64; trazo irregular con un filtro `feTurbulence` + `feDisplacementMap`) y `wf/tema-balsamiq.dg`. El CSS no cambia la geometría: el ancho de los textos sigue midiéndose con Helvetica.

## 8. Antes y después

Figura con contenedores (de `fig13-1.dg`), sin directivas ni coordenadas:

```
import shapes
diagram fig13-1 frame style=oreilly caption="Figure 13-1. Sharding in Redis using hash slots"
  row gap=130
    col gap=110
      box ord "Order[OrderNum]" w=200 h=54 aligny=n0
      box inv "Invoice[InvNum]" w=200 h=54 aligny=n2
    group slots "Hash slots" pad=16 gap=10
      row gap=26
        box n0 "Node 0" w=120 h=50
        db d0 "0-4095" w=90 h=62
      row gap=26
        box n1 "Node 1" w=120 h=50
        db d1 "4096-\n8191" w=90 h=62
      ...
  ord.right -> slots.left@=n0 label="Hash (OrderNum)\nmod 16384" sw=8
  n0 -> d0 sw=3
  n1 -> d1 sw=3
```

Figura libre (de `so/cap10-01.dg`), con contenedores en vez de tres `at=`:

```
import so
diagram cap10-01 style=silberschatz size=11 caption="Un sistema de archivos: colección de archivos y estructura de directorios."
  col gap=46 align=center
    box t "Sistema de\nArchivos" light w=110 h=44
    row gap=60
      box a "Colección de\narchivos" light w=110 h=44 sw=2
      box b "Estructura de\ndirectorios" light w=110 h=44 sw=2
  t.bottom -> a.top elbow
  t.bottom -> b.top elbow
```

Pantalla (de `wf/ui-shop.wf`): no cambia salvo `diagram`/`screen` como encabezado y `w` en vez de `width`.

## 9. Migración de los archivos actuales

Hecha por `tools/conv.py` sin tocar los originales: `@frame`/`@style`/`@size`/`@caption` pasan al encabezado `diagram`; `width`/`height` a `w`/`h`; `box ... noborder` a `sw=0`; `none` a `fill=none`; `dashed` a `dash="6 4"`; los usos de `def` con id posicional pasan a `id=`; los tamaños de letra dominantes por archivo pasan a un `style local`. `tools/build.sh` convierte, renderiza y compara con `tools/compare.py` (`python3 compare.py cap3-10` muestra las diferencias de un archivo).

## 10. Pendiente

- `silberschatz` está verificado contra 149 referencias; `oreilly` (`lib/styles.dg`) no tiene referencias y sus valores salen de `diagram.js`.
- Diferencia conocida: `ldx`/`ldy` (desplazar una etiqueta) se eliminaron; `cap3-10` es el único archivo afectado.
- `alignx=` sobre un contenedor padre no está soportado (`fig2-8b`).
- La librería `ui` de wireframe (§4) falta: `wf/` aún no se ha probado.
- Los `diagram` de wireframe con `node`, `action` (hexágono con salidas `ok`/`error`) y `frame ids=` se tratan como `def` de la librería `flow`.

## Filas de `data`
- Dentro de `data` cada línea es una fila, sin palabra clave: `Pedro Perez, 10, #4C78A8`.
- Los valores se separan por comas y se recortan los espacios. No hacen falta comillas salvo que el valor tenga coma, comillas o espacios al borde: `"Monitor, 24", $210` (dentro de comillas, `\"` y `\\` escapan).
- Una línea que empieza con `# ` es comentario. Un valor puede empezar con `#` (colores) si no lo sigue un espacio.
- La cabecera acepta espacios o comas: `data ventas mes, total, producto`.
- Una fila con menos valores que columnas deja las que faltan con el nombre de la columna y el número de fila.

## Gráficas: marcas definidas por el usuario
Una gráfica es `chart` con hijos que son marcas (`bar`, `line`, `pie`, …) o `rule`. Una marca es cualquier nombre `X` para el que existe `def mark_X`; no hay que recompilar nada.

- `def mark_X rows y0 ymax`: la plantilla. Dentro del marco de la gráfica están `pl pt pw ph` (origen y tamaño del área de trazado); `repeat $rows` recorre las filas. Columnas disponibles por fila: `px` (posición en x, 0..1), `bo bw` (desplazamiento y ancho de la banda), `v` (valor), `b` (base apilada), `c` (color), `nx px2 v2 b2` (siguiente punto del mismo grupo; `nx` es 1 si existe), `t` (valor formateado), `s` (tamaño normalizado de `size=`), `lo hi` (columnas `lo=` y `hi=`). Para y: `pt+ph-($v-$y0)/($ymax-$y0)*ph`.
- `def mark_hX`: variante para `flip` (opcional).
- `data mark_X zero, band, arrange, rows, frame` (opcional, una fila): `zero=1` base en cero; `band=1` eje x por categorías; `arrange` = dodge|stack|overlay; `rows` = generador de filas `point` (predeterminado), `box` (cuartiles) o `share` (porciones: `a0 a1 lg c t v`); `frame` = `plot` (ejes) o `bare` (sin ejes). Si falta, se usan los valores predeterminados.
- Funciones de `expr` útiles para marcas: `sin`, `cos`, `pi`, `min`, `max`, `abs`.
- Capas con columnas extra: `size=col`, `lo=col`, `hi=col`; también `where`, `agg`, `bin`, `sort`, `color`, `labels`.
- Ejemplos: `lib/marks.dg` (donut, lollipop, step, bubble, errorbar), `charts/marcas-extra.dg` y `charts/marca-propia.dg` (marcas definidas en el mismo archivo).
- Lo que sí requiere C++: un generador de filas nuevo (hoy `point`, `box`, `share`).

## Gráficas: nuevas capacidades

- **Fechas**: si todos los valores de `x` son `AAAA-MM-DD` o `AAAA-MM`, el eje es temporal (ticks por día, mes o año).
- **Marcas con tamaño**: si una capa tiene `size=` y no parte de cero (`point`, `bubble`), los ejes `x` e `y` numéricos se amplían un 8 % para que las marcas no salgan del área de trazado. Las marcas del eje `x` numérico se reducen mientras no quepan en el ancho disponible (ancho del gráfico menos márgenes y leyenda).
- **Escalas**: `xscale=log` / `yscale=log` en `chart`. No se admiten con marcas que parten de cero (barras).
- **Capas con tablas propias**: cada capa hereda `data x y value color where` de la primera; las transformaciones (`fit`, `smooth=N`, `cumulative`, `ecdf`, `density`) producen su propia tabla sin cambiar los puntos.
- **Columnas extra**: `extra="a,b"` agrega esas columnas a las filas de la marca (velas, bullet, gantt).
- **`arrange=fill`**: apilado al 100 %. **`arrange=waterfall`** (marca `waterfall`, opción `total`).
- **Generador `cell`**: mapa de calor, atributos `x`, `y` (categorías) y `value`; la rampa sale de `data ramp c`.

## Gráficas: canales visuales (`color`, `size`, `shape`, `alpha`, `dash`, `sw`)

Una sola regla: si el valor es el **nombre de una columna** se mapea; si no, es un **literal fijo**.

    point data=cars x=peso y=mpg color=#E45756 size=6 shape=diamond   # fijo
    point data=cars x=peso y=mpg color=origen shape=origen size=hp    # por ítem
    point data=cars x=peso y=mpg color=hp:q4 size=hp:q4               # por clases

| Canal | Fijo | Columna categórica | Columna numérica | Columna de literales |
|---|---|---|---|---|
| `color` | `#hex` | `data palette` (agrupa y lleva leyenda) | rampa `data ramp c` entre mínimo y máximo | `#hex` tal cual |
| `fill` | `#hex` o `none` | como `color` (parte la serie) | como `color` | `#hex` tal cual |
| `size` | número (radio en px) | — | `smin + f·(smax-smin)`, con `f = |v|/máximo`; `sizes=min,max` en `chart` (por defecto `4,18`) | — |
| `shape` | nombre de `data shapes` | una forma por valor, en orden de aparición | solo con clases | nombres de `data shapes` tal cual |
| `alpha` | opacidad 0..1 | — | rango `alphas=min,max` (por defecto `0.2,1`), mínimo→máximo de la columna | — |
| `dash` | elemento de `data dashes` (`none`, `"6 3"`...) | un patrón por valor, en orden de aparición | solo con clases | patrones de `data dashes` tal cual |
| `sw` | grosor de línea en px | — | rango `widths=min,max` (por defecto `1,5`) | — |

- **Relleno y contorno**: con `fill=` el interior sale de `fill` y `color` pasa a ser el contorno (de ancho `sw`, 1 por defecto); sin `color`, el contorno es del color del relleno. `fill=none color=origen` da puntos huecos. Sin `fill`, `color` pinta todo, como siempre. Lo leen `bar`, `hbar`, `area` y `point`; la leyenda muestra el color del relleno.
- **Esquemas de color**: `import colores` (`lib/graficas/colores.dg`) trae rampas y paletas de ColorBrewer y viridis, y en el `chart` se eligen con `ramp=YlOrRd` (continuo y clases) o `palette=Set2` (categorías). Un esquema propio es un `data NOMBRE c` con un `#hex` por fila (de claro a oscuro en las rampas); sin `ramp=` ni `palette=` rigen `data ramp` y `data palette`. Un nombre inexistente da error.
- `data shapes` y `data dashes` (en `lib/graficas/plots.dg`) son las listas de formas y de patrones; el dibujo de las formas está en `def glyph`. Agregar una forma o un patrón es una línea en la lista (y un bloque en `glyph` para las formas). `point` usa `glyph`.
- Qué marcas leen cada canal: `alpha` en barras, áreas, puntos, burbujas, líneas, escalones y piruletas; `dash` y `sw` en líneas, escalones, piruletas y radar (y `sw` en la cruz de `point`); `shape` solo en `point`; `size` en `point` y `bubble`. Una marca propia los usa con `$alpha`, `$dash`, `$sw`, `$rad` y `$sh`.
- **Leyenda**: una guía por variable, con el nombre de la columna como título (en un gráfico circular, la columna `x`); los canales que usan la misma columna (y los mismos cortes) comparten guía, como `color=origen shape=origen` (formas de color) o `color=hp:q4 size=hp:q4` (círculos de color y tamaño crecientes). Cada canal con columna tiene guía: categórico o por clases, una marca por categoría o clase; numérico continuo (`color`, `size`, `alpha`, `sw`), 2-3 valores redondos entre el mínimo y el máximo (`color=hp size=hp` comparten guía; la marca de `size` se reduce a 9 px como máximo, la de `alpha` es un cuadro gris con esa opacidad y la de `sw` una línea gris con ese grosor). El margen derecho crece con la etiqueta más larga.
- **Facetas**: `facet=campo` reparte la gráfica en paneles (`cols=N` en la rejilla); `facet=fila,columna` los cruza como `facet_grid`: una fila de paneles por valor de la primera columna y una columna por valor de la segunda, con una tira gris pegada arriba (valor de la columna) y otra a la derecha (valor de la fila), celdas que son solo el área de trazado (fondo blanco y borde gris, sin tarjeta), los ejes solo en el borde izquierdo e inferior y la leyenda una sola vez arriba a la derecha. Las combinaciones sin datos quedan vacías. `gap=N` (10 por defecto) separa las celdas. Con el indicador `vstrip` el texto de las filas se gira hacia arriba y la tira derecha queda angosta (18 px). En este modo `w` y `h` son el área de trazado de cada celda (los márgenes y las tiras se suman), se ignoran `title`, `xtitle` y `ytitle`, y solo vale con gráficas verticales (no `flip` ni circulares). Ejemplos: `charts/facetas-2d.dg`, `charts/facetas-2d-leyenda.dg`.
- **Series**: cada canal asignado a una columna categórica (`color`, `dash`, `shape`) parte la serie, y `by=columna[,columna]` aporta columnas sin estilo ni leyenda. La serie es la combinación de esas columnas, sin repetir las que coinciden: `dash=ciudad` dibuja una línea por ciudad con distinto trazo, `color=ciudad dash=tipo` una por cada combinación (color por ciudad, trazo por tipo; dos guías), `by=ciudad` varias líneas iguales sin leyenda. También agrupan el apilado, el lado a lado y `agg`. Los canales numéricos (`size`, `alpha`, `sw`) no parten series.
- El atributo `alpha=` también existe en las primitivas de `shape` (`rect`, `ellipse`, `line`, `path`, `poly`) y `dash=none` equivale a trazo continuo.
- `label` admite `rotate=grados` (negativo = hacia arriba), alrededor de su centro.
- **Clases**: `canal=columna:espec` agrupa los valores numéricos antes de mapearlos (el color usa `k/(n-1)` de la rampa, el tamaño el mismo valor entre `smin` y `smax`, la forma la clase `k`). `espec`: `qN` cuantiles (`q4` cuartiles), `eN` intervalos iguales, `a,b,c` cortes manuales, `sd` media ± 1 y 2 desviaciones. Con clases la leyenda muestra los rangos.
- Si no hay columna con ese nombre, `size=` exige un número y `color=` un `#hex`; `shape=` debe estar en `data shapes`.
- Cambio respecto a antes: `color=` sobre una columna **numérica** ahora es una rampa continua (antes eran categorías por valor), y una columna de `#hex` o una fija ya no agrupa ni genera leyenda. Los atajos (`points`, `bubbles`...) aceptan los seis canales y `sizes`, `alphas`, `widths`.
- Las filas de marca incluyen las columnas `rad`, `sh`, `alpha`, `dash` y `sw` (ya resueltas), además de `s` (fracción 0..1 de `size`) que ya existía.
- Ejemplo: `charts/atributos.dg`.
- **`wrap`** en `data mark_X` (radar): el eje x es circular; la marca recibe `yt` y `xt` para anillos y ejes.

## Layout implícito

Un `diagram` con dos o más elementos y sin `at`, `alignx`, `aligny` ni `around` apila los elementos en columna, igual que `col`. La separación se ajusta con `gap=` en el `diagram` (por defecto 12). El tamaño de cada elemento sale del texto, así que `w` y `h` solo se escriben para forzarlo. Si algún elemento tiene posición explícita, el diagrama sigue siendo un lienzo libre como antes.

## Macro `graph`

Una línea por relación; los nodos se crean al mencionarlos y se ordenan por niveles según las flechas.

    graph [direction=down|right] [gap=N] [sep=N]
      id "Etiqueta" [atributos de box]      declara un nodo
      a -> b -> c                           cadena de flechas
      a -> "texto" b                        flecha con etiqueta
      a "A" -> b "B"                        declara y conecta en la misma línea

- El id va primero y la etiqueta entre comillas después; sin etiqueta se usa el id.
- Los atributos extra del nodo (`light`, `w=`, ...) pasan a la caja.
- Las flechas hacia atrás (ciclos) dan la vuelta con `margin`.
- Se expande a `col`/`row` + `box` + conectores `elbow`, así que se puede mezclar con el DSL normal.
- Los ids no deben coincidir con el nombre de un `def`.

## Macros `tree`, `sequence` y `datatable`

**`tree [direction=down|right] [gap=N] [sep=N]`**: la jerarquía sale de la indentación; una línea por nodo, `id "Etiqueta" [atributos de box]`. Hace falta una sola raíz. Se expande a `col`/`row` anidados y conectores `elbow`.

**`sequence [step=N]`**: diagrama de secuencia.

    sequence
      c "Cliente"            participante (opcional; sin declarar se usa el id)
      c -> s "petición"      mensaje
      s --> c "respuesta"    respuesta punteada

Los participantes salen en orden de aparición. Un mensaje necesita dos participantes distintos. Usa `box`, `spacer` y conectores con posiciones calculadas.

**`datatable [zebra] [w=N]`**: filas CSV (igual que `data`, sin comillas obligatorias); la primera fila es el encabezado. Todas las filas deben tener el mismo número de celdas; una celda vacía se escribe `""`. Se expande a un `grid` de `box`. El nombre `table` ya lo usan `blocks.dg` y `ui.dg`.

El parser lee como filas CSV los hijos de `data` y de cualquier cabecera registrada en `DSL::raws()` (hoy `data` y `datatable`).

## Macro `form` (pantallas compactas para wf)

Es `screen` con un cuerpo de una línea por elemento, sin comillas y sin atributos. Los argumentos de la primera línea (`id=`, `w=`, `phone`, el título) pasan tal cual a `screen`. Requiere `import ui`.

    form id=p1 "Registro" w=270
      Nombre               campo
      Contraseña *         campo con contraseña
      v País               select
      ? Buscar             campo de búsqueda
      [x] Acepto           checkbox marcado   ([ ] sin marcar)
      @ ¿Ya tienes cuenta? enlace
      = Hola, María        título
      ~ nota               nota
      --                   separador
      > Crear cuenta       botón a la derecha
      >> Cancelar          botón con borde, a la derecha
      >< Centro            botón centrado

Genera el mismo SVG que escribir `screen` con `field`, `button`, `note`... a mano. Lo que no cubre (tarjetas, filas, imágenes) se puede escribir en la sintaxis larga dentro de un `screen` normal. Como las filas se leen como CSV, una coma dentro de un texto se conserva (`= Hola, María`).

## Atajos de gráficas

Cada atajo es una macro que se expande a un `data` anónimo y un `chart` con una capa. La sintaxis larga sigue disponible y se puede mezclar; el atajo solo cubre una gráfica de una tabla.

    bars "Ventas" w=400 h=260 labels
      mes, total              primera fila: encabezado (CSV, como data)
      Ene, 10
      Feb, 12

Nombres: `bars lines points areas pies donuts boxes heatmap radars waterfalls steps bubbles lollipops`, y `quick MARCA` para cualquier `def mark_MARCA`. Si existe un `def` con el mismo nombre, el `def` gana, pero el parser lee como filas CSV los hijos de estos nombres, así que un `def` propio con hijos no debe llamarse igual (por eso `stairs` y no `steps`, que ya usa `shop.dg`).

- **Columnas por defecto**: `x` = primera, `y` = segunda, `color` = tercera si no es numérica. Con una sola columna, cuenta (`agg=count`). `heatmap`: `x`, `y` y `value` son las tres primeras. Se cambian con `x= y= color= value=`.
- **Atributos de la capa** (`x y value color agg sort bin arrange where lo hi size extra`) y los de `chart` (`w h subtitle xtitle ytitle yscale xscale facet cols ...`) se escriben en la misma línea; el título es el primer texto entre comillas.
- **Opciones sueltas**: `stack`, `fill`, `labels`, `total`, `cumulative`, `ecdf`, `density`, `flip`, `commas`, `zero`; `dots` agrega puntos, `fit` una recta de ajuste y `smooth=N` una media móvil.
- Una celda vacía se escribe `""`. Todas las filas deben tener tantas celdas como el encabezado.

## `form`: tarjetas, filas, imágenes y más

Además de los elementos simples (campo, `v` select, `?` búsqueda, checkbox, `@` enlace, `=` título, `~` nota, `--`, botones `>` `>>` `><`), `form` entiende:

    [img]  [img 80]        imagen (alto opcional)       [thumb]  miniatura
    (nuevo)                etiqueta (badge)
    - texto                texto simple (una línea)     ~~~ 3  barras de relleno (lines n=3)
    | ? Buscar | > Ir      fila: elementos separados por |, cada uno en sintaxis corta
    + Título               tarjeta; las líneas indentadas debajo son su contenido
    +gray Título           tarjeta con tono (+gray sin título también vale)

Solo una tarjeta (`+`) puede tener líneas indentadas debajo. Cada atajo genera el mismo SVG que su forma larga (`card`, `row`, `image`, `badge`, `text`, `lines`). Lo que necesite atributos finos se escribe con la sintaxis larga dentro de un `screen` normal.

Cambio en el parser: los hijos de las cabeceras "crudas" (`data`, `datatable`, `form`, los atajos de gráficas) guardan su indentación en `Node.indent`.

## Reparto con `flex`, componentes abiertos, `include` y `alias prefix`

Cambios opcionales que no alteran ningún diagrama existente (los 210 dg, 113 wf y 46 charts salen idénticos). Están en C++ (`src/`), en `js_port/` y en el
bundle, con el mismo resultado.

- **`flex` / `flex=N`**: un hijo de `row` o de `col` con la palabra `flex` (o `flex=N`) se reparte el espacio libre del eje principal en proporción a N, como CSS flex.
  Lo fija un `w=` (en una fila) o un `h=` (en una columna), y `flex=0` no reparte. A diferencia de `grow` (solo filas, y también "ocupa todo el ancho"), funciona en columnas.
  `weights=` también se acepta en una `col` o un `box`. Para que haya espacio libre, la `col` o el `box` necesita un `h=` mayor que su contenido.
- **`*` al final de una `def`**: `def nombre a b *`. El componente ignora los atributos con nombre que no declara, las palabras sueltas sin comillas
  que no son parámetros y los textos que sobran, en vez de reenviarlos o dar error. Sirve para leer texto escrito para otro programa.
- **`include archivo clave=valor`** (en el texto de un diagrama): se sustituye por el contenido de `archivo` (lo entrega el resolvedor, como en `import`).
  En el archivo, `clave=$nombre` toma el valor del parámetro y desaparece si no se dio. Un `include` puede incluir otros.
- **`alias prefix swl_`** (en una librería): en el texto del diagrama, no en el cuerpo de las `def`, el nombre `x` significa `swl_x` si existe esa `def`.
  Así una librería puede dar sentido a nombres que ya usa textod (`row`, `line`, `label`, `list`, `table`…) sin que sus propios cuerpos los confundan.
- El bundle exporta `Diagrams.renderSwl(texto, resolvedor, tema, nombre)`: envuelve el texto de un `.swl` de Swinglet en `import swl` + `diagram nombre style=ui`.
  La librería `swl` (`lib/wireframes/swl.dg`) dibuja los widgets de Swinglet.
  Un dibujo es un estado de la ventana: antes de dibujar, `swlStates()` (en `tools/bundle.js`) deja una sola página en cada `stack` (la que lleva `show`, la que elige el
  `tabbedpane`/`buttongroup` de `choice=`, `value=` o la primera), convierte `options="a,b"` en segmentos con el elegido presionado (`option "x" show`) y, con
  `menu "Archivo" show` (o `include menus/archivo.swl show`; `swlStates` expande antes los `include` con la misma regla de parámetros), dibuja el menú abierto como superposición (`dropdown`: `at=` bajo su título, después del `frame`, así que nada se mueve; solo un menú abierto a la vez). Si la barra no es lo primero de la ventana, o el texto tiene otros elementos en la raíz, cae a `popup`, que sí ocupa espacio bajo la barra (`menuitem`, `checkboxmenuitem`, `menuline`).
