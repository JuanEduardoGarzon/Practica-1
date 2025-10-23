#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Limite de 8kb de memoria por linea
#define MAX_LINE 8192

// --- Normaliza título ---
void normalize_title(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_size; i++) {
        char c = src[i];
        if (c == '\r' || c == '\n') break;    // Para al encontrar fin de línea
        if (c == '"' || c == '\'') continue;  // Omite comillas
        if (c >= 'A' && c <= 'Z') c += 32;    // Convierte a minúsculas
        dst[j++] = c;                         // Copia carácter normalizado
    }
    while (j > 0 && dst[j - 1] == ' ') j--;   // Elimina espacios al final
    dst[j] = '\0';                            // Termina string
} 

// --- Hash DJB2 ---
unsigned long hashFunction(const char *str) {
    unsigned long hash = 5381;                // Semilla mágica del algoritmo numero primo que funciona
    int c;                  
    while ((c = *str++))                      // Recorre cada carácter del string
        hash = ((hash << 5) + hash) + c;      // hash * 33 + c formular que distribuye bien los hashes
    return hash;
}

// --- Robust CSV parser for indexer ---
float extract_rating_from_csv(const char *line) {
    int column = 0;
    int in_quotes = 0;
    const char *start = line;
    
    for (const char *p = line; *p; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;
        } else if (*p == ',' && !in_quotes) {   // Controla si estamos dentro de comillas
            // Cuando encontramos coma FUERA de comillas osea End of column
            if (column == 6) { // This is the rating column (review/score)
                // Extract the rating value
                char rating_str[32];
                size_t length = p - start;     // Changed to size_t
                if (length > 0 && length < sizeof(rating_str)) {
                    strncpy(rating_str, start, length);
                    rating_str[length] = '\0';
                    return atof(rating_str);   // Convierte string a float
                }
            }
            start = p + 1;  // Prepara para siguiente columna
            column++;
        } else if ((*p == '\n' || *p == '\r') && !in_quotes) {
            // // Fin de línea - procesa última columna End of line
            if (column == 6) {
                // Mismo proceso que arriba para la última columna
                char rating_str[32];
                size_t length = p - start;  // Changed to size_t
                if (length > 0 && length < sizeof(rating_str)) {
                    strncpy(rating_str, start, length);
                    rating_str[length] = '\0';
                    return atof(rating_str);
                }
            }
            break;
        }
    }
    return 0.0f; // Si no encuentra rating, devuelve 0
}

int main() {
    FILE *csv = fopen("/home/juanegr/Documents/Sistemas Operativos/Practica 1/Practica1/data/books_reduced.csv", "r");
    if (!csv) {
        perror("Books_rating.csv");
        return 1;
    }

    FILE *out = fopen("/home/juanegr/Documents/Sistemas Operativos/Practica 1/Practica1/src/output/books_index_stream.bin", "wb");
    if (!out) {
        perror("books_index_stream.bin");
        fclose(csv);
        return 1;
    }

    char line[MAX_LINE];        // Buffer para línea actual
    char normalized[1024];      // Título normalizado
    long position;              // Posición en el CSV
    unsigned long hash;         // Hash calculado
    float rating;               // Rating extraído
    size_t count = 0;           // Contador de registros
    size_t zero_ratings = 0;    // Contador de ratings cero

    // Saltar encabezado
    if (fgets(line, sizeof(line), csv) == NULL) {  // Fixed warning
        printf("Error: Archivo vacío o error de lectura\n");
        fclose(csv);
        fclose(out);
        return 1;
    }

    while ((position = ftell(csv)), fgets(line, sizeof(line), csv)) {
        char line_copy[MAX_LINE];
        strcpy(line_copy, line);

        // --- Extraer título (columna 1) ---
        int col = 0;
        int in_quotes = 0;
        char *title_start = line_copy;
        char *title_end = NULL;
        
        for (char *p = line_copy; *p; p++) {
            if (*p == '"') {
                in_quotes = !in_quotes;
            } else if (*p == ',' && !in_quotes) {
                if (col == 1) { // Title column
                    title_end = p;
                    break;
                }
                title_start = p + 1;
                col++;
            }
        }
        
        if (!title_end) continue;
        
        // Extract and clean title
        *title_end = '\0';
        if (title_start[0] == '"' && title_end > title_start + 1 && title_end[-1] == '"') {
            title_start++;
            *(title_end - 1) = '\0';
        }
        
        normalize_title(title_start, normalized, sizeof(normalized));
        if (strlen(normalized) == 0) continue;
        
        hash = hashFunction(normalized);

        // --- Extraer rating usando el parser robusto ---
        rating = extract_rating_from_csv(line);
        
        // Debug: track zero ratings
        if (rating == 0.0f) {
            zero_ratings++;
            if (zero_ratings <= 5) { // Show first 5 zero ratings for debugging
                printf("DEBUG Zero rating: title='%s', line: %.100s\n", normalized, line);
            }
        }

        // Validar rango
        if (rating < 0.0f) rating = 0.0f;
        if (rating > 5.0f) rating = 5.0f;

        // --- Escribir al archivo binario ---
        fwrite(&hash, sizeof(hash), 1, out);
        fwrite(&position, sizeof(position), 1, out);
        fwrite(&rating, sizeof(rating), 1, out);
        count++;

        if (count % 100000 == 0)
            printf("%zu registros procesados...\n", count);
    }

    printf("✅ Indexación finalizada: %zu registros escritos.\n", count);
    printf("⚠️  %zu registros con rating 0.0 (revisar parsing)\n", zero_ratings);

    fclose(csv);
    fclose(out);
    return 0;
}