# Guía de Contribución y Flujo de Trabajo

### 1. Convención de Ramas
Crear siempre ramas desde `main` actualizado usando prefijos
- `featnombre-tarea` Nuevas funcionalidades (ej. `featsniper-bolt`).
- `fixnombre-tarea` Corrección de errores (ej. `fixcalculo-balas`).
- `refactornombre-tarea` Mejoras internas de código.
- `chorenombre-tarea` Tareas operativas o dependencias.

### 2. Ciclo de Trabajo
1. Actualizar `main` `git checkout main && git pull origin main`
2. Crear rama `git checkout -b featmi-tarea`
3. Commits con mensajes descriptivos.
4. Antes de abrir PR, sincronizar con `main`
   ```bash
   git fetch origin
   git merge origin main


###########   DDAY CHILE   #################