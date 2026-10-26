# Progresiones P.A. / P.G. para Casio fx-CG50

Add-in (.g3a) con 13 formulas de progresiones aritmeticas y geometricas.
Muestra el **procedimiento paso a paso** (formula -> sustitucion -> resultado).

## Uso en la calculadora
| Tecla | Menu | Entrada de datos | Resultado |
|---|---|---|---|
| UP / DOWN | elegir formula | cambiar de simbolo (t1, n, r...) | desplazar |
| EXE | abrir formula | **calcular** | volver |
| EXIT | salir | volver al menu | volver |
| 0-9 . (-) / | - | escribir el valor (acepta fracciones: `1/2`) | - |
| DEL / AC | - | borrar caracter / borrar campo | - |

Arriba aparece la categoria y la descripcion de la formula seleccionada.
Los valores escritos se conservan en cada formula mientras el programa este abierto.

## Formulas
**P.A.**: tn, n, Sn, Sn con r, r, t1, termino central.
**P.G.**: tn, Sn (incluye q=1), S infinita (|q|<1), Pn, q, n.

## Obtener el .g3a desde GitHub
1. Crea un repositorio y sube TODO el contenido de esta carpeta (respetando `.github/`).
2. Pestana **Actions** -> workflow "Compilar g3a" -> se ejecuta solo al hacer push.
   La primera vez tarda ~30 min (compila el cross-compiler); luego queda en cache.
3. Al terminar, abre la ejecucion y descarga el artefacto **progresiones-g3a** (`progresiones.g3a`).

## Instalar en la calculadora
1. Conecta la fx-CG50 al PC con el cable USB -> en la calculadora elige **USB Flash** (F1).
2. Copia `progresiones.g3a` a la raiz de la unidad "CASIO" que aparece.
3. Expulsa la unidad de forma segura y desconecta. Aparece en el menu principal como **Progresiones**.

## Compilar en tu PC (opcional)
Necesitas `sh3eb-elf-gcc`, `mkg3a` y `libfxcg`. Luego: `make FXCGSDK=/ruta/a/libfxcg`.
`make test` prueba la logica de calculo en el PC sin necesidad de la calculadora.

## Estructura
- `src/core.h`  formulas, formato de numeros, parser (sin codigo de Casio)
- `src/main.c`  interfaz de la calculadora
- `tests/`      pruebas en PC
- `assets/`     iconos del menu (92x64 BMP)
- `.github/workflows/build.yml`  compilacion automatica

## Agregar una formula nueva
1. Escribe `f_xxx(const double *v, Out *o)` en `core.h` (copia una existente).
2. Anadela a la tabla `FORMS[]` y sube `NFORM`.
