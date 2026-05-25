# Vulkan Deferred Engine

<p>
	<strong>Motor de renderizado diferido basado en Vulkan, desarrollado en C++.</strong>
</p>

<p>
	<img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C">
	<img alt="Vulkan" src="https://img.shields.io/badge/Vulkan-Renderer-A41E22">
	<img alt="Visual Studio" src="https://img.shields.io/badge/Visual%20Studio-2022-5C2D91">
	<img alt="Platform" src="https://img.shields.io/badge/Platform-x64-0A7EA4">
</p>

Documentacion: https://luisenriquecabelloblanco.github.io/VulkanDeferredEngine/

---

## Vista General

> [!IMPORTANT]
> La solucion no incluye solo el motor. Tambien incluye 3 aplicaciones de ejemplo listas para compilar y ejecutar.

| Proyecto | Tipo | Descripcion |
| --- | --- | --- |
| VulkanRenderEngine | Libreria estatica | Motor de renderizado. |
| RenderAplication | Aplicacion | Ejemplo base para iniciar rapido. |
| FireflyExample | Aplicacion | Ejemplo autocontenido con assets propios. |
| MainHallExample | Aplicacion | Ejemplo autocontenido con assets propios. |

## Dependencias

| Dependencia | Estado |
| --- | --- |
| Visual Studio 2022 (toolset v143, x64) | Requerida |
| Vulkan SDK (VULKAN_SDK configurada) | Requerida |
| SDL2 | Integrada en la SDK de Vulkan |
| GLM | Integrada en la SDK de Vulkan |

## Estructura Del Repositorio

| Carpeta/Archivo | Contenido |
| --- | --- |
| engineSrc/ | Codigo fuente y cabeceras publicas del motor |
| projects/ | Proyectos de Visual Studio (motor + ejemplos) |
| shaders/ | Shaders necesarios para compilar el motor |
| utils/ | Scripts auxiliares (shader_compilation.bat) |
| Doxyfile, mainpage.md | Configuracion y contenido de documentacion |

### Proyectos En projects/

- VulkanRenderEngine/: proyecto de libreria estatica del motor.
- RenderAplication/: ejemplo base.
- FireflyExample/: ejemplo autocontenido (assets/, src/).
- MainHallExample/: ejemplo autocontenido (assets/, src/).

## Configuracion De La Solucion

- Configuraciones disponibles: Debug|x64 y Release|x64.
- VulkanRenderEngine genera librerias en lib/x64/(Debug|Release).
- Los tres ejemplos enlazan contra esa libreria y dependen del proyecto del motor.
- La compilacion de shaders se ejecuta mediante eventos de build ya configurados.

## Compilar El Motor

1. Abrir VulkanRenderEngine.sln en Visual Studio.
2. Seleccionar configuracion Debug o Release y plataforma x64.
3. Ejecutar Build Solution o compilar solo VulkanRenderEngine.

## Ejecutar Las Aplicaciones De Ejemplo

1. En el explorador de solucion, establecer como Startup Project uno de estos:
	 - RenderAplication
	 - FireflyExample
	 - MainHallExample
2. Ejecutar con F5 o Ctrl+F5.

> [!NOTE]
> Visual Studio compila automaticamente el motor y la aplicacion seleccionada.

## Nota Rapida De Recursos

Cada ejemplo usa su propia carpeta de assets. El directorio de trabajo de cada aplicacion esta configurado en su carpeta de proyecto correspondiente.

## Release (GitHub)

En el apartado Release del repositorio de GitHub hay una SDK del motor lista para usar que incluye:

- El motor ya compilado.
- Dependencias integradas.
- Una solucion de Visual Studio con una aplicacion de ejemplo totalmente configurada para empezar a usar el motor.
