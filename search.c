#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>   //Funciones de caracteres (tolower)
#include <sys/shm.h> //sys/shm.h: Memoria compartida entre procesos
#include <unistd.h>  //Funciones del sistema (usleep, getpid)


#define SHM_SIZE 16384  //Tamaño de memoria compartida (16KB)
#define MAX_LINE 8192   //Tamaño máximo de línea CSV (8KB)

typedef struct {
    char command[256];      // Comando del UI al motor
    char results[12288];    // Resultados del motor al UI (12KB)
    int ready;              // Bandera de sincronización
} shared_data_t;

#define INDEX_FILE "/home/juanegr/Documents/Sistemas Operativos/Practica 1/Practica1/src/output/books_index_stream.bin"
#define CSV_FILE "/home/juanegr/Documents/Sistemas Operativos/Practica 1/Practica1/data/books_reduced.csv"

void normalize_title(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_size; i++) {
        char c = src[i];
        if (c == '\r' || c == '\n') break;
        if (c == '"' || c == '\'') continue;
        if (c >= 'A' && c <= 'Z') c += 32;
        dst[j++] = c;
    }
    while (j > 0 && dst[j - 1] == ' ') j--;
    dst[j] = '\0';
}

//Convierte string en número único para búsquedas rápidas
unsigned long hashFunction(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

void extract_csv_columns(const char *line, char *columns[], int max_cols) {
    int col = 0;
    int in_quotes = 0;
    const char *start = line;
    
    // Inicializa todas las columnas como strings vacíos
    for (int i = 0; i < max_cols; i++) {
        columns[i] = malloc(1);      // Pide 1 byte de memoria
        columns[i][0] = '\0';        // Pone el carácter nulo (string vacío)
    }
    
    for (const char *p = line; *p && col < max_cols; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;  // Controla estado dentro de comillas
        } else if (*p == ',' && !in_quotes) {
            // Encontró separador de columna FUERA de comillas
            int length = p - start;
            free(columns[col]);           // Libera string vacío anterior
            columns[col] = malloc(length + 1);  // Asigna memoria para columna
            strncpy(columns[col], start, length);  // Copia contenido
            columns[col][length] = '\0';  // Termina string
            col++;                        // Siguiente columna
            start = p + 1;                // Nueva posición inicial
        } else if ((*p == '\n' || *p == '\r') && !in_quotes) {
            // Fin de línea - procesa última columna
            int length = p - start;
            free(columns[col]);
            columns[col] = malloc(length + 1);
            strncpy(columns[col], start, length);
            columns[col][length] = '\0';
            col++;
            break;
        }
    }
}

void perform_search(const char *title, float rating, char *results) {
    FILE *idx = fopen(INDEX_FILE, "rb"); //abre binario
    FILE *csv = fopen(CSV_FILE, "r");    //abre csv originalv
    
    if (!idx || !csv) {
        strcpy(results, "Error: No se pudieron abrir los archivos de datos");
        return;
    }

    char normalized[256];
    normalize_title(title, normalized, sizeof(normalized));  // Normaliza título
    unsigned long search_hash = hashFunction(normalized);    // Calcula hash

    unsigned long hash;
    long position;
    float stored_rating;
    char line[MAX_LINE];
    int found = 0;
    
    char *temp_results = malloc(10240);                      // Buffer temporal para resultados
    temp_results[0] = '\0';

    fgets(line, sizeof(line), csv);                          // Salta encabezado del CSV

     while (fread(&hash, sizeof(hash), 1, idx) == 1 &&        // Lee hash
           fread(&position, sizeof(position), 1, idx) == 1 && // Lee posición
           fread(&stored_rating, sizeof(stored_rating), 1, idx) == 1) { // Lee rating
        
        if (hash == search_hash && stored_rating == rating) {
            // Coincidencia
            long current_pos = ftell(csv);  // Guarda posición actual
            fseek(csv, position, SEEK_SET); // Salta a posición del registro
            
            if (fgets(line, sizeof(line), csv)) {
                char *columns[10];
                extract_csv_columns(line, columns, 10);  // Parsea CSV
                
                // Extrae columnas específicas
                char *book_title = columns[1];   // Título
                char *profile = columns[4];      // Usuario
                char *summary = columns[8];      // Resumen
                
                if (book_title && strlen(book_title) > 0) {
                    char book_info[200];
                    
                    //Remueve comillas de los campos
                    if (book_title[0] == '"' && book_title[strlen(book_title)-1] == '"') {
                        memmove(book_title, book_title + 1, strlen(book_title) - 2);
                        book_title[strlen(book_title) - 2] = '\0';
                    }
                    if (profile && profile[0] == '"' && profile[strlen(profile)-1] == '"') {
                        memmove(profile, profile + 1, strlen(profile) - 2);
                        profile[strlen(profile) - 2] = '\0';
                    }
                    if (summary && summary[0] == '"' && summary[strlen(summary)-1] == '"') {
                        memmove(summary, summary + 1, strlen(summary) - 2);
                        summary[strlen(summary) - 2] = '\0';
                    }
                    
                    // Acorta resumen si es muy largo
                    char short_summary[40] = "";
                    if (summary && strlen(summary) > 0) {
                        strncpy(short_summary, summary, sizeof(short_summary) - 1);
                        short_summary[sizeof(short_summary) - 1] = '\0';
                        if (strlen(summary) > 35) {
                            strcpy(short_summary + 32, "...");
                        }
                    }
                    
                    snprintf(book_info, sizeof(book_info), 
                            "%d. %s | %s | %s\n", 
                            found + 1,
                            book_title, 
                            profile && strlen(profile) > 0 ? profile : "Anonymous",
                            short_summary);
                    
                    // Agrega al buffer de resultados si hay espacio
                    if (strlen(temp_results) + strlen(book_info) < 10240 - 100) {
                        strcat(temp_results, book_info);
                        found++;
                    } else {
                        // Buffer lleno - agrega mensaje y termina
                        char trunc_msg[80];
                        snprintf(trunc_msg, sizeof(trunc_msg), 
                                "... (showing first %d results)\n", found);
                        strcat(temp_results, trunc_msg);
                        break;
                    }
                }
                // Libera memoria de columnas
                for (int i = 0; i < 10; i++) {
                    free(columns[i]);
                }
            }
            fseek(csv, current_pos, SEEK_SET);  // Restaura posición en CSV
        }
    }

    if (found == 0) {
        snprintf(results, 12288, "❌ No se encontraron resultados para '%s' con rating %.1f", title, rating);
    } else {
        char header[150];
        snprintf(header, sizeof(header), 
                "✅ Se encontraron %d resultados para '%s' (rating %.1f):\n\n", 
                found, title, rating);
        
        strcpy(results, header);
        strcat(results, temp_results);
    }

    free(temp_results);   // Libera buffer temporal
    fclose(idx);          // Cierra archivos
    fclose(csv);
}

int main() {
    printf("🚀 Motor de búsqueda iniciado (PID: %d)\n", getpid());
    
    key_t key = ftok(".", 'S');   // Crea clave única para memoria compartida
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);   // Crea/obtiene segmento
    shared_data_t *shared = (shared_data_t*) shmat(shmid, NULL, 0);   // Conecta
    
    shared->ready = 1;           // Inicialmente listo
    shared->command[0] = '\0';   // Limpia comando
    shared->results[0] = '\0';   // Limpia resultados

    printf("📊 Memoria compartida lista. Esperando comandos...\n\n");
    
    while (1) {
        if (strlen(shared->command) > 0 && !shared->ready) {
            // Hay un nuevo comando del UI
            if (strncmp(shared->command, "search:", 7) == 0) {
                // Comando de búsqueda
                char *title = shared->command + 7;    // Extrae título
                char *rating_str = strchr(title, ':');  // Encuentra separador
                if (rating_str) { 
                    *rating_str = '\0';                // Separa título
                    float rating = atof(rating_str + 1);        // Convierte rating
                    perform_search(title, rating, shared->results);   // Ejecuta búsqueda
                }
            } 
            else if (strcmp(shared->command, "exit") == 0) {
                // Comando de salida
                printf("👋 Cerrando motor de búsqueda...\n");
                break;
            }
            
            shared->ready = 1;   // Indica que terminó de procesar
        }
        usleep(100000);         // Espera 100ms para no consumir CPU
    }
    
    shmdt(shared);                // Desconecta memoria compartida
    shmctl(shmid, IPC_RMID, NULL);// Elimina segmento
    return 0;
}