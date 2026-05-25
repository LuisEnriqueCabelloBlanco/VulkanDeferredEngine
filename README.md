# Vulkan Deferred Engine

Motor de renderizado diferido basado en Vulkan, desarrollado en C++.

El repositorio contiene una solucion de Visual Studio compuesta por 4 proyectos:

- VulkanRenderEngine: motor de renderizado (libreria estatica).
- RenderAplication: aplicacion de ejemplo base.
- FireflyExample: ejemplo autocontenido con assets propios.
- MainHallExample: ejemplo autocontenido con assets propios.

La solucion no contiene solo el motor: incluye tambien 3 aplicaciones de ejemplo listas para compilar y ejecutar desde Visual Studio.

## Dependencias

- Visual Studio 2022 (toolset v143, plataforma x64).
- Vulkan SDK (variable de entorno VULKAN_SDK configurada).

## Estructura Del Repositorio

- engineSrc/: codigo fuente y cabeceras publicas del motor.
- projects/: proyectos VS de motor y ejemplos.
	- VulkanRenderEngine/: proyecto de libreria estatica del motor.
	- RenderAplication/: ejemplo base.
	- FireflyExample/: ejemplo autocontenido (assets/, src/).
	- MainHallExample/: ejemplo autocontenido (assets/, src/).
- shaders/: shaders necesarios para compilar el motor.
- utils/: scripts auxiliares (shader_compilation.bat).
- Doxyfile y mainpage.md: configuracion y contenido de documentacion.

## Configuracion De La Solucion

- Configuraciones: Debug|x64 y Release|x64.
- El proyecto VulkanRenderEngine genera librerias en lib/x64/(Debug|Release).
- Los tres ejemplos enlazan contra esa libreria y tienen dependencia de proyecto al motor dentro de la solucion.
- La compilacion de shaders se lanza desde eventos de build ya configurados.

## Compilar El Motor

1. Abrir VulkanRenderEngine.sln en Visual Studio.
2. Seleccionar configuracion Debug o Release y plataforma x64.
3. Compilar la solucion (Build Solution) o compilar solo VulkanRenderEngine.

## Ejecutar Cada Aplicacion De Ejemplo

1. En el explorador de solucion, establecer como Startup Project uno de estos:
	 - RenderAplication
	 - FireflyExample
	 - MainHallExample
2. Ejecutar con F5 o Ctrl+F5.

Visual Studio compila automaticamente el motor y la aplicacion seleccionada.

## Nota Rapida De Recursos

Para ejecutar correctamente, cada ejemplo debe usar su propia carpeta de assets. El directorio de trabajo de cada aplicación está configurado en el directorio de proyecto correspondiente.

Documentacion: https://luisenriquecabelloblanco.github.io/VulkanDeferredEngine/
