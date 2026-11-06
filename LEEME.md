# Orbital Reverb: de HTML a plugin VST3

Esta carpeta contiene tu Orbital Reverb convertida en un plugin de verdad para Cubase o cualquier otro DAW.
Suena igual que tu HTML: mismos controles, mismos rangos y mismo volumen. También tiene el mismo diseño.

No necesitas instalar JUCE ni Visual Studio: **GitHub fabrica el plugin por ti en la nube, gratis.**

---

## Qué hay dentro

| Carpeta / archivo | Para qué sirve |
|---|---|
| `Source/` | El plugin: el sonido y el diseño, traducidos desde tu HTML |
| `PARA-GITHUB/build.yml` | Las instrucciones que sigue GitHub para fabricar el plugin |
| `Tests/` | Pruebas automáticas (comprueban que suena como el HTML) |
| `Original/` | Tu HTML original, como referencia |
| `CMakeLists.txt` | La ficha del proyecto (nombre, fabricante, formatos) |
| `captura-plugin.png` | Así se ve el plugin |

---

## Paso a paso (unos 20 minutos, solo con el navegador)

1. **Crea una cuenta gratis** en <https://github.com>.
2. Arriba a la derecha pulsa **+** y luego **New repository**.
   - Nombre: `orbital-reverb`
   - Marca **Public**: las compilaciones son ilimitadas y gratis. *Private* también vale, pero tiene un límite mensual.
   - Pulsa **Create repository**.
3. En la página que aparece, pulsa el enlace **uploading an existing file**.
   - Descomprime este ZIP.
   - Abre la carpeta `OrbitalReverb` y **arrastra todo lo que hay dentro** a la ventana del navegador: `CMakeLists.txt`, `LEEME.md` y las carpetas `Source`, `Tests`, `Original` y `PARA-GITHUB`.
   - Pulsa **Commit changes**.
4. Crea el archivo de compilación:
   - Pulsa **Add file** y luego **Create new file**.
   - En el nombre escribe exactamente esto: `.github/workflows/build.yml`
   - Abre `PARA-GITHUB/build.yml` con el Bloc de notas (Windows) o TextEdit (Mac). Copia todo su contenido y pégalo en GitHub.
   - Pulsa **Commit changes**.
5. Abre la pestaña **Actions**. Verás **Compilar Orbital Reverb** en marcha, con un círculo amarillo.
   - Tarda unos 10–15 minutos.
   - Cuando salga el **check verde**, entra en él y baja hasta **Artifacts**.
6. Descarga **Orbital-Reverb-Windows** u **Orbital-Reverb-Mac**, según tu ordenador.

---

## Instalar en Windows (Cubase)

1. Descomprime el archivo descargado. Obtendrás la carpeta **`Orbital Reverb.vst3`**.
2. Cópiala en `C:\Program Files\Common Files\VST3`. Windows te pedirá permiso: acepta.
3. Abre Cubase. Si no aparece, ve a **Estudio → Administrador de plug-ins VST** y pulsa actualizar, o reinicia Cubase.
4. La encontrarás en **Efectos → Reverb → Orbital Reverb**.

## Instalar en Mac (Cubase, Logic…)

1. Descomprime **Orbital-Reverb-Mac**. Dentro hay dos ZIP; descomprímelos con doble clic.
2. En el Finder, ve a **Ir → Ir a la carpeta…** y escribe `~/Library/Audio/Plug-Ins/VST3`. Copia ahí **`Orbital Reverb.vst3`**.
3. Para Logic o GarageBand, copia **`Orbital Reverb.component`** en `~/Library/Audio/Plug-Ins/Components`.
4. El plugin no está firmado con un certificado de Apple, así que el Mac lo bloquea al principio. Abre la app **Terminal** y pega estas dos líneas:

   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Orbital Reverb.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Orbital Reverb.component"
   ```

5. Reinicia tu DAW.

---

## Si algo sale en rojo en Actions

Entra en el paso que aparece en rojo, copia el texto del error y pégalo en el chat. Lo arreglamos.

## Diferencias con el HTML

- Se han quitado los botones **Play** y **Piano**. En el DAW, el sonido llega de tu pista.
- Puedes **agrandar la ventana** arrastrando la esquina inferior derecha.
- Todos los controles se pueden **automatizar** desde Cubase y se guardan con el proyecto.
- Un mismo ajuste genera siempre la misma reverb. Así, lo que exportas suena exactamente igual que lo que escuchas.

## Cambios futuros

Si modificas algo y vuelves a subirlo a GitHub, el plugin se vuelve a fabricar solo.

## Licencia

El plugin está hecho con JUCE, que es gratis para uso personal. Si algún día quieres vender tus plugins, revisa las condiciones en <https://juce.com/get-juce>.
