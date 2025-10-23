# Practica-1
Porgrama de busquedas de reviews de libros

Debido al peso del dataset de 1.4Gb no se puede subir al repositorio sin embargo aqui se encuentra el Link
https://www.kaggle.com/datasets/mohamedbakhet/amazon-books-reviews
El dataset de las reviews debera ir en un carpeta data con esta estructura

book_search_system/
├── Makefile                 # Automatización de compilación
├── indexer.c               # Creador del índice binario
├── search.c                # Motor de búsqueda (servidor)
├── ui.c                    # Interfaz de usuario (cliente)
├── data/
│   └── books_reduced.csv   # Base de datos de libros
└── src/
    └── output/
        └── books_index_stream.bin  # Índice generado

Este programa se compila mediante el Makefile y se corre el proceso search.c y el ui.c en terminales diferentes

# Compilar todos los componentes
make run

# Terminal 1 - Iniciar motor de búsqueda
./search

# Terminal 2 - Iniciar interfaz de usuario  
./ui

# Comandos de makfile
make all      # Compilar todos los componentes
make run      # Compilar y mostrar instrucciones
make search   # Mostrar instrucciones de ejecución
make clean    # Limpiar archivos compilados
make help     # Mostrar ayuda
