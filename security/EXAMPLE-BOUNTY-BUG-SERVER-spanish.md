# CatchChallenger — Bug Bounty del servidor de juego (guía de alcance y superficie de ataque)

Este documento está dirigido a **investigadores de seguridad independientes**. Te indica **dónde
buscar** y **qué recompensamos** — deliberadamente **no** describe ninguna
vulnerabilidad concreta. El servidor ha sido auditado y reforzado; el objetivo de este programa
es confirmarlo y recompensar a cualquiera que encuentre algo que se nos haya escapado.

> Un informe válido es aquel que **tú** descubres y demuestras. Aquí no listamos problemas
> conocidos, porque no hay ninguno del que tengamos constancia — si encuentras uno, es nuevo y
> se recompensa. Lee el código, compila el servidor, atácalo y muéstranos un reproductor.

https://github.com/alphaonex86/CatchChallenger/

---

## 1. Objetivo

El objetivo dentro del alcance es el **servidor de juego independiente** compilado desde `server/cli/`
(`catchchallenger-server-cli`), el servidor de bucle de eventos de un solo hilo que acepta
conexiones de jugadores en el **puerto TCP del juego**. La única entrada del atacante que consideramos son
**bytes enviados a través de ese socket TCP** por un cliente conectado (o que se está conectando).

Compila una instancia local para atacarla — no se requieren paquetes adicionales más allá de una cadena de
herramientas C++ y CMake:

```sh
cmake -S server/cli -B build/server-cli
cmake --build build/server-cli -j
```

Ejecútalo contra un datapack y un `server-properties.xml` (automatic_account_creation value="true"; `mainDatapackCode` selecciona el maincode, `compression=none`
facilita la lectura de los paquetes en el cable). La compilación con base de datos en fichero (`FILE_DB`)
no necesita ningún demonio SQL y es la más sencilla de poner en marcha.

**Prueba solo contra tu propia instancia.** No ataques los servidores públicos, a otros
jugadores ni a ningún host `*.herman-brule.com`.

---

## 2. Alcance

**Dentro del alcance** — código alcanzable desde el puerto TCP del juego:

* El encuadre (framing) del protocolo de cable y el despacho:
  `general/base/ProtocolParsing*.cpp`, `server/base/ClientNetworkRead*.cpp`.
* Los manejadores por paquete y la lógica de juego que invocan:
  `server/base/`, `server/base/ClientEvents/`, `server/crafting/`, `server/fight/`,
  y el motor compartido en `general/base/` y `general/fight/`.
* El estado del servidor y la persistencia alcanzables a través de esos manejadores (inventario del jugador,
  dinero, monstruos, posición, misiones, clanes, intercambios, tiendas, fábricas, plantas).

**Fuera del alcance:**

* Bibliotecas de terceros incluidas (`general/blake3`, `general/hps`,
  `general/libxxhash`, `general/libzstd`, `general/tinyXML2`,
  `client/libqtcatchchallenger/libtiled`, `libogg`/`libopus`/`libopusfile`). Repórtalas
  upstream.
* El cliente, la herramienta de administración con GUI y el sistema de compilación.
* Los servidores del clúster login / master / gateway (programa aparte; puede tener su propio
  bounty más adelante).
* Hallazgos que requieran un **servidor**, datapack o configuración maliciosos o modificados — el
  atacante solo controla un socket de cliente.
* DoS puramente volumétrico (saturar ancho de banda/conexiones). La
  amplificación algorítmica a partir de *un único paquete pequeño* (ver §3) **sí** está dentro del alcance.
* Cualquier cosa solo alcanzable con el flag de compilación `CATCHCHALLENGER_HARDENED` activado — el
  servidor de producción se compila con él **desactivado**. Prueba con los flags por defecto (desactivado).
* Acción remota: para mejorar el rendimiento, algunas acciones se permiten en el mapa local.

---

## 3. Qué recompensamos (clases de vulnerabilidad)

Pagamos por un defecto alcanzable por TCP que, disparado por bytes del cliente, provoque cualquiera de:

1. **Seguridad de memoria** — lectura o escritura fuera de límites, uso después de liberación (use-after-free),
   doble liberación (double-free), lectura no inicializada, desbordamiento de pila, o un uso incorrecto de
   contenedor/iterador que corrompa el heap. Demuéstralo con un sanitizer (ASan/UBSan) o una traza de valgrind.
2. **Caída / excepción no gestionada** — cualquier entrada que aborte o termine el
   proceso del servidor, o una excepción de C++ no capturada que se propague fuera de un manejador.
3. **Bloqueo / bucle infinito / amplificación de CPU** — un único paquete acotado que
   sature un núcleo o nunca retorne, negando el servicio a otros jugadores.
4. **Integridad económica** — crear, duplicar o destruir objetos, dinero,
   monstruos, recetas, plantas o XP sin el coste o la propiedad legítimos;
   cualquier desbordamiento/subdesbordamiento de enteros en un precio, cantidad, recuento o índice que
   produzca bienes gratuitos o negativos.
5. **Elusión de propiedad / autorización** — actuar sobre, leer o mutar el estado de otro
   jugador o de otro personaje; usar o vender algo que no posees;
   seleccionar/eliminar un personaje que no posees.
6. **Elusión de las reglas del mundo** — moverse a través de colisiones o salientes (ledges) de un solo sentido, teletransportarse
   o alcanzar una casilla que las reglas prohíben.
7. **Corrupción de estado persistido** — entrada que deja los datos guardados del personaje/cuenta
   incorrectos (filtración entre jugadores, blob malformado, valores fantasma) después de que
   termine la conexión.

Un hallazgo del tipo "el servidor registró un error y me desconectó" o "el servidor
ignoró mi paquete malformado y siguió funcionando" **no** es una vulnerabilidad — esa es la
defensa prevista (el servidor puede expulsar al infractor o ignorar de forma segura la
entrada; ambas opciones son aceptables). Solo recompensamos los resultados enumerados arriba.

---

## 4. La superficie de ataque — zonas que sondear

Cada paquete de juego es `[packetCode:1]( [queryNumber:1] )( [dataSize:4 LE] )[data]`.
`queryNumber` está presente en las consultas/respuestas (código ≥ 0x80) y en la respuesta 0x7F;
`dataSize` está presente solo en los paquetes **dinámicos** (aquellos cuyo tamaño fijo registrado
es el centinela "no fijo" en `ProtocolParsingGeneral.cpp` — comprueba esa tabla
antes de encuadrar un paquete; equivocarte simplemente estanca el parser). Todos los enteros
multibyte son little-endian. El despacho de manejadores reside en
`server/base/ClientNetworkReadMessage.cpp` (mensajes) y
`ClientNetworkReadQuery.cpp` (consultas) — léelos primero; son la puerta de entrada a
todas las zonas de abajo.

Conéctate, completa el handshake hasta tener un personaje seleccionado y luego pon a prueba cada zona.
Para cada paquete, varía: el tamaño declarado frente a los bytes reales, cada campo de longitud/recuento/índice
en sus extremos (0, 1, máximo, máximo±1), ids que no existen o que no posees,
y el **estado** desde el que lo envías (fuera de combate, no en una tienda, sin clan, etc.).

| Zona | Paquetes / manejadores | Dónde |
|---|---|---|
| Encuadre y despacho | prefijo de tamaño, números de consulta, códigos desconocidos, bytes previos al login | `general/base/ProtocolParsingInput*.cpp`, `ClientNetworkRead*.cpp` |
| Movimiento y mapa | mover, teletransportar, tomar objeto, visibilidad del mapa | `ClientNetworkReadMessage.cpp`, `ClientEvents/LocalClientHandlerMove.cpp`, `LocalClientHandler.cpp` |
| Chat | local / todos / clan / privado, tamaños de texto | `ClientNetworkReadMessage.cpp`, `ClientBroadCast*.cpp` |
| Inventario / objetos | usar, destruir, usar sobre monstruo | `ClientEvents/LocalClientHandlerObject.cpp` |
| Tienda y fábrica | comprar, vender, comprar/vender/listar en fábrica | `ClientEvents/LocalClientHandlerShop.cpp` |
| Fabricación y plantas | usar receta, plantar semilla, recoger planta | `server/crafting/` |
| Misiones | iniciar / cancelar / finalizar / siguiente-paso, requisitos | `ClientEvents/LocalClientHandlerQuest.cpp` |
| Clan | crear / abandonar / disolver / invitar / aceptar-rechazar | `ClientEvents/LocalClientHandlerClan.cpp` |
| Intercambio | solicitar / añadir objeto y monstruo / finalizar / cancelar | `ClientEvents/LocalClientHandlerTrade.cpp` |
| Combate | solicitar combate, usar habilidad, cambiar/mover monstruo, aprender habilidad, evolución, huir, curar | `server/fight/`, `general/fight/CommonFightEngine.cpp` |
| Personaje y datapack | añadir / seleccionar / eliminar personaje, listar/sincronizar ficheros del datapack | `ClientNetworkReadQuery.cpp`, `server/base/ClientLoad/` |

Presta especial atención a: la aritmética sobre `price × quantity` controlado por el atacante,
`count`, `stepCount`, e índices de array/bitmap derivados de campos de id de 8 o 16 bits;
el acceso a contenedores (`.at()`, `operator[]`, `erase`, `front`/`begin`) cuya
precondición depende de un campo del paquete; y cualquier manejador cuyo efecto dependa de la
posición actual del jugador o del estado del juego.

---

## 5. Reglas de participación

* Ataca **solo** una instancia del servidor que ejecutes tú mismo. Nunca toques producción, a otros
  jugadores ni infraestructura compartida.
* Nada de ingeniería social, ataques físicos ni ataques al hosting, correo,
  DNS o cuentas del proyecto.
* No exfiltres ni destruyas datos que no te pertenezcan; usa cuentas de prueba desechables.
* Danos un tiempo razonable para corregir antes de cualquier divulgación pública.

---

## 6. Cómo enviar un informe

Envía un informe privado (contacto a través del sitio canónico) que contenga:

1. **Clase** (de §3) y un resumen de impacto de una línea.
2. **Reproductor exacto**: los bytes en bruto / la secuencia de paquetes, el estado inicial, y un
   script o pasos para dispararlo contra una instancia de `server/cli` recién compilada.
3. **Evidencia**: el backtrace de la caída, la salida del sanitizer/valgrind, o el estado del servidor
   antes/después que demuestre el impacto.
4. El **hash del commit** que probaste y tus flags de compilación.

Los informes que solo señalan una línea de código "que parece arriesgada" sin un reproductor
funcional no son elegibles. Marca el fichero:línea si puedes — pero es el reproductor lo
que gana la recompensa.

---

## 7. Niveles de recompensa (orientativo)

| Severidad | Ejemplos | Recompensa |
|---|---|---|
| Crítica | Corrupción de memoria remota con control, caída de todo el servidor a partir de un paquete, duplicación ilimitada de objetos/dinero | nivel máximo |
| Alta | Lectura/escritura fuera de límites, caída/bloqueo fiable, corrupción de estado entre jugadores, elusión de propiedad | alta |
| Media | Abuso económico acotado, elusión de límites, elusión de reglas del mundo con impacto limitado | media |
| Baja / Info | Carencias menores de endurecimiento, problemas latentes no alcanzables en la compilación de producción | ninguna |

La severidad final y el importe quedan a discreción de los mantenedores, en función del impacto real
y de la reproducibilidad. **Los problemas duplicados o ya corregidos no se recompensan** —
y precisamente por eso este documento te entrega el mapa pero no el tesoro.
