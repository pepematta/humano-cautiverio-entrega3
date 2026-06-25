# El Humano en Cautiverio — Explicación del proyecto (Entrega 3)

## Qué es

Una pieza de **fisicalización de datos con interacción tangible y sonificación**, desarrollada para el curso IIC2026 Visualización de Información (PUC Chile). Es la tercera entrega de un proyecto semestral sobre uso del tiempo humano.

El proyecto compara cómo una persona (o una sociedad) distribuye su tiempo diario frente a un patrón de referencia: el del **humano cazador-recolector** ("primitivo"), que representó el modo de vida de nuestra especie durante el ~95% de su historia. La tesis, heredada de entregas anteriores, usa la metáfora del "humano en cautiverio": así como un animal en cautiverio deja de expresar sus conductas naturales (déficit conductual), el humano moderno se ha alejado de su distribución natural del tiempo.

## Las tres partes del sistema

El sistema tiene tres componentes que funcionan acoplados:

1. **Pieza física (fisicalización de salida):** tres servomotores, cada uno con una aguja o indicador que **rota** según el valor de su categoría. La rotación de cada servo representa una de las tres categorías de uso del tiempo (es el equivalente físico de los medidores circulares de la web).

2. **Web (capa digital + espejo visual + sonificación):** una página HTML que muestra las mismas tres categorías como tres medidores circulares tipo termómetro (con aguja), reproduce la sonificación, y se comunica con el Arduino.

3. **Sonificación:** un sonido continuo cuya consonancia/disonancia refleja qué tan lejos está la distribución mostrada respecto al patrón primitivo.

## Las tres categorías (datos)

Las tres categorías son las del **OECD Time Use Database**, elegidas porque son comparables entre países:

- **Trabajo** (trabajo remunerado + estudio)
- **Ocio** (tiempo libre, descanso, vida social)
- **Cuidado personal** (dormir, comer, higiene; incluye sueño)

Cada categoría tiene tres valores posibles:
- **Mis datos:** ingresados manualmente con sliders (representan el día del usuario).
- **Primitivo:** valor fijo del cazador-recolector (trabajo ~240 min, ocio ~450 min, cuidado ~690 min), estimado de estudios etnográficos.
- **Por país:** promedios OCDE de 12 países (Chile, México, EE.UU., Francia, Alemania, España, Italia, Japón, Corea, Noruega, Reino Unido, Canadá).

## Cómo se interactúa

**Pulsador físico (la interacción tangible principal):** un botón conectado al Arduino. Mientras está **suelto**, los servos y los medidores muestran "Mis datos" (o el país elegido). Mientras se **mantiene presionado**, todo cambia al patrón "Humano primitivo": los servos físicos se reacomodan (sus agujas rotan a la nueva posición), el fondo de la web vira de crema a verde, y el título cambia de "Modo: Mis datos" a "Modo: Humano primitivo". Al soltar, vuelve. Ese contraste —ver y oír cómo se reacomoda el día— es el núcleo de la pieza.

**Sliders (web):** tres deslizadores para ingresar manualmente los minutos de cada categoría del propio día.

**Selector de países (web):** chips con bandera; al elegir uno, los tres medidores (y los servos) viajan a los promedios de ese país. Volver a tocarlo, o mover un slider, regresa a "Mis datos".

**Botón de sonido (web):** activa o silencia la sonificación.

## Cómo funciona la sonificación

Se calcula un **índice de naturalidad**: para cada una de las tres categorías se mide la desviación normalizada entre el valor mostrado (mis datos o país) y el valor primitivo; se promedian las tres (peso igual). Índice = 1 significa idéntico al primitivo; índice = 0, muy lejano.

Ese índice controla el **intervalo entre dos osciladores**:
- Cercano al primitivo → **quinta justa** (702 cents), intervalo consonante y estable.
- Lejano del primitivo → se desliza hacia el **tritono** (600 cents), disonante y tenso.

Es un barrido continuo: cuanto más se aleja la distribución del patrón natural, más áspero suena. Si el día fuera idéntico al primitivo, sonaría la consonancia plena. (Esto retoma el uso de intervalos consonante/disonante de la entrega anterior, ahora ligado al contraste con el primitivo.)

## Arquitectura técnica

**Hardware:** Arduino UNO + 3 servomotores SG90 (pines D9, D10, D11) + 1 pulsador (pin D2, con resistencia pull-up interna, a GND). Los servos se alimentan con fuente externa de 5V/≥2A con GND común al Arduino (no desde el 5V del UNO, que no da la corriente).

**Firmware (Arduino):** recibe por serial (9600 baud) tres valores crudos separados por coma (`trabajo,ocio,cuidado`), los guarda como "estado real", los mapea a ángulos de servo según un máximo por categoría, y rota los servos suavemente hasta esos ángulos. El pulsador alterna entre ese estado real y un estado primitivo fijo en código. El Arduino informa el cambio de modo imprimiendo por serial (`-> CAZADOR-RECOLECTOR` / `-> TU DIA REAL`).

**Web (HTML autocontenido):** un único archivo, sin frameworks ni dependencias salvo Google Fonts. Usa:
- **SVG** para los medidores circulares (arco de 270°, aguja rotada según el valor).
- **Web Audio API** para la sonificación (dos osciladores sinusoidales).
- **Web Serial API** para conectarse al Arduino (solo Chrome/Edge de escritorio): envía el estado mostrado a los servos y lee los mensajes del pulsador para cambiar de modo en pantalla.
- Sin almacenamiento de navegador; todo el estado vive en variables JS.

Para probar sin Arduino, mantener la **barra espaciadora** simula el pulsador.

## Identidad visual

Hereda la estética de la primera entrega: fondo crema (#FAFAF7), tipografía serif (Source Serif 4) para títulos, sans (DM Sans) para cuerpo, monoespaciada (JetBrains Mono) para cifras, y una paleta natural por categoría. Minimalista: solo los elementos necesarios.

## Flujo de datos (resumen)

```
Sliders / Selector de país  →  estado "mostrado" en la web
                            →  medidores SVG (aguja)
                            →  sonificación (índice de naturalidad → intervalo)
                            →  serial → Arduino → servos físicos

Pulsador físico  →  Arduino alterna real/primitivo
                 →  serial → web cambia título + fondo + agujas + sonido
```

## Fuentes de datos

- **OECD Time Use Database** (promedios por país, población 15–64 años).
- **Patrón cazador-recolector:** estimaciones de estudios etnográficos (Lee sobre !Kung San; Dyble et al. sobre Agta; Pontzer et al. sobre actividad física Hadza; Sahlins y Marlowe sobre presupuestos de tiempo).

## Notas de honestidad metodológica

- "Cuidado personal" en el dataset público de la OCDE **incluye el sueño**, por eso es la categoría más estable entre países (~660–700 min).
- Los valores por país son representativos de las publicaciones OCDE, redondeados.
- Los datos personales se ingresan a mano; en discusiones previas se exploró automatizarlos (movimiento vía Apple Salud, trabajo vía Google Calendar), pero el tiempo de pantalla no es accesible por API en iOS. Esa limitación de captura —el teléfono solo "ve" una fracción del día— es parte de la reflexión del proyecto.
