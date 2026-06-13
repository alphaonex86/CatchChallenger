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

El primer objetivo dentro del alcance es el **servidor de juego independiente** compilado desde `server/cli/`
(`catchchallenger-server-cli`), el servidor de bucle de eventos de un solo hilo que acepta
conexiones de jugadores en el **puerto TCP del juego**. La entrada del atacante que consideramos son
**bytes enviados a través de un socket TCP** por un **cliente de juego** conectado (o que se está
conectando) — contra el servidor de juego independiente, o contra los servidores de cara al cliente del
clúster: login, gateway y game-server-alone (ver *Los servidores del clúster* más abajo). El master y los
enlaces entre nodos del clúster son solo de red interna y están fuera del alcance (§2).

Compila una instancia local para atacarla — no se requieren paquetes adicionales más allá de una cadena de
herramientas C++ y CMake:

```sh
cmake -S server/cli -B build/server-cli
cmake --build build/server-cli -j
```

Ejecútalo contra un datapack y un `server-properties.xml` (automatic_account_creation value="true"; `mainDatapackCode` selecciona el maincode, `compression=none`
facilita la lectura de los paquetes en el cable). La compilación con base de datos en fichero (`FILE_DB`)
no necesita ningún demonio SQL y es la más sencilla de poner en marcha.

### Los servidores del clúster

Además del servidor independiente, otros cuatro binarios componen el despliegue
multiservidor (*clúster*). Hablan el MISMO protocolo de cable (§4); cada uno analiza
una porción distinta:

| Binario | Fuente | BD | Par no confiable | ¿En alcance? |
|---|---|---|---|---|
| `catchchallenger-server-login` | `server/login/` | PostgreSQL/MySQL | cliente de juego | **sí** — login / autocreación de cuenta, alta / baja / selección de personaje, y luego un proxy transparente cliente↔juego |
| `catchchallenger-gateway` | `server/gateway/` | ninguna | cliente de juego | **sí** — un proxy fino cliente↔backend + descarga/sincronización de la lista de ficheros del datapack |
| `catchchallenger-game-server-alone` | `server/game-server-alone/` | PostgreSQL/MySQL | cliente de juego | **sí** (puerto de cliente) — los mismos manejadores de cliente que `server/cli` |
| `catchchallenger-server-master` | `server/master/` | PostgreSQL/MySQL | **nodos** login / game-server | **no** — solo red interna del clúster (ver §2) |

El **master** solo lo alcanzan otros nodos del clúster a través de la red interna
VPS / LAN y nunca se expone a clientes ni a internet — así que él, y todo enlace entre
nodos (incluido el `LinkToMaster` del game-server), está **fuera del alcance**. El
bounty se centra en los tres servidores **de cara al cliente** de arriba.

`login`, `master` y `game-server-alone` se compilan **solo** contra un backend SQL real
(`-DCATCHCHALLENGER_DB_POSTGRESQL=ON` o `-DCATCHCHALLENGER_DB_MYSQL=ON`) y solo
funcionan **juntos como clúster**; el `gateway` no usa BD. La forma más sencilla de
levantar todo tal como lo probamos — un PostgreSQL efímero, los binarios conectados
entre sí con un token de master compartido, y un cliente `qtcpu800x600` real conducido
de extremo a extremo hasta el mapa — es `test/testingcluster.py` (clúster) más
`test/testinggateway.py` (gateway); los casos de robustez por servidor están en
`test/testingclustersecurity.py`. Léelos para conocer los puertos exactos, las claves
de `server-properties.xml`/`login.xml`/`master.xml`, el token compartido y el relevo
login → master → game-server.

```sh
cmake -S server/master -B build/master -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/login  -B build/login  -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/game-server-alone -B build/gsa -DCATCHCHALLENGER_DB_POSTGRESQL=ON
cmake -S server/gateway -B build/gateway        # sin BD
cmake --build build/master -j && cmake --build build/login -j && \
  cmake --build build/gsa -j && cmake --build build/gateway -j
```

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
* **Servidor login** (`server/login/`) — el handshake previo al login, el login y
  autocreación de cuenta, y los manejadores de alta / baja / selección de personaje
  (`EventLoopClientLoginSlaveProtocolParsing.cpp`), más el proxy transparente
  cliente↔juego en que se convierte tras la selección.
* **Gateway** (`server/gateway/`) — el handshake previo al login, la descarga /
  sincronización de la lista de ficheros del datapack, y el paso directo de move / chat
  / select al backend (`EventLoopClientLoginSlaveProtocolParsing.cpp`, `DatapackDownloader*.cpp`).
* **Game-server-alone** (`server/game-server-alone/`) — sus manejadores de cara al
  cliente son el mismo código `server/base/ClientNetworkRead*` que `server/cli` (en
  alcance). Su análisis de las respuestas del enlace al master (`LinkToMaster*.cpp`) está
  **fuera del alcance** — ver abajo.

**Fuera del alcance:**

* Bibliotecas de terceros incluidas (`general/blake3`, `general/hps`,
  `general/libxxhash`, `general/libzstd`, `general/tinyXML2`,
  `client/libqtcatchchallenger/libtiled`, `libogg`/`libopus`/`libopusfile`). Repórtalas
  upstream.
* El cliente, la herramienta de administración con GUI y el sistema de compilación.
* **El servidor master (`server/master/`) y todo enlace entre nodos** — login↔master,
  game-server↔master, y el análisis de respuestas `LinkToMaster*` del game-server. Solo
  hablan por la red interna VPS / LAN del clúster: el master nunca se enlaza a una
  interfaz pública, nunca lo alcanza un cliente ni internet, así que un atacante remoto
  no puede llegar a él. **Fuera del alcance.**
* Hallazgos que requieran un **binario de servidor**, **datapack** o **configuración**
  maliciosos o modificados — el atacante solo controla un socket de *cliente* (juego /
  login / gateway / game-server). Manipular el binario o la configuración de otro nodo
  está fuera del alcance.
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

### Servidores del clúster

El clúster habla el mismo encuadre; solo cambia el punto de despacho por binario.
Levanta el clúster (`test/testingcluster.py` / `test/testingclustersecurity.py`) para
que funcione el relevo login → master → game-server, y luego sondea cada listener
**de cara al cliente** de abajo. (El master y los enlaces entre nodos solo reciben
bytes de otros nodos del clúster en la red interna — fuera del alcance, §2 — así que no
se listan.)

| Servidor | Punto de despacho | Manejadores alcanzables (por estado) | Dónde |
|---|---|---|---|
| login | `parseInputBeforeLogin` / `parseQuery` | `0xA0` handshake; `0xA8` login; `0xA9` crear-cuenta; `0xAD` stat; luego (Logged) `0xAA` alta-personaje, `0xAB` baja-personaje, `0xAC` selección-personaje | `server/login/EventLoopClientLoginSlaveProtocolParsing.cpp` |
| gateway | `parseInputBeforeLogin` / `parseMessage` / `parseQuery` | `0xA0` handshake; `0xA1` lista-ficheros datapack; `0xAC` select-reconexión; `0x02`/`0x03` paso directo move/chat | `server/gateway/EventLoopClientLoginSlaveProtocolParsing.cpp`, `DatapackDownloader*.cpp` |
| game-server-alone | `ClientNetworkRead*` (igual que `server/cli`) | todo manejador de cliente del servidor de juego independiente | `server/base/` |

Aspectos específicos del clúster en los que insistir: los bytes de longitud de pseudo /
login / contraseña en los paquetes login `0xAA`/`0xA8`/`0xA9`; los índices
`charactersGroupIndex` / `profileIndex` / `skinId` / `monsterGroupId` en login
`0xAA`/`0xAB`/`0xAC`; y el bucle de la lista de ficheros del datapack del gateway
(`number_of_file`, `textSize` por fichero, `partialHash`), su búsqueda
`serverReconnectList[charactersGroupIndex][uniqueKey]`, y la ruta de reensamblado de
nombres / hashes de `DatapackDownloader*`.

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
   script o pasos para dispararlo contra una instancia de `server/cli` recién compilada (o,
   para un hallazgo del clúster, el clúster levantado con `test/testingcluster.py`).
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
