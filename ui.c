#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 16384  //Tamaño de memoria compartida (16KB)

typedef struct {
    char command[256];      // Comando UI → Motor
    char results[12288];    // Resultados Motor → UI  
    int ready;              // Bandera de sincronización
} shared_data_t;

//Limpia el buffer de entrada después de scanf() para evitar problemas con entradas posteriores
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void display_menu() {
    printf("\n");
    printf("╔══════════════════════════════╗\n");
    printf("║    📚 BUSCADOR DE LIBROS     ║\n");
    printf("╠══════════════════════════════╣\n");
    printf("║ 1. 🔍 Buscar libro           ║\n");
    printf("║ 2. ❌ Salir                  ║\n");
    printf("╚══════════════════════════════╝\n");
    printf("Opción: ");
}

void send_search_command(shared_data_t *shared) {
    char title[200];  // Buffer para título (dejando espacio para prefijo/sufijo)
    float rating;
    
    printf("\n🎯 BÚSQUEDA DE LIBROS\n");
    printf("─────────────────────\n");
    
    printf("📖 Título: ");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        printf("❌ Error leyendo título\n");
        return;
    }
    title[strcspn(title, "\n")] = 0; // Elimina el newline de fgets()
    
    // Calcula longitud máxima permitida para el título
    // "search:" = 7 chars + ":x.x" = 4 chars + null = 1 char
    // Total: 256 - 12 = 244 chars para título
    size_t max_title_len = sizeof(shared->command) - 12; // 256 - 12 = 244
    
    if (strlen(title) > max_title_len) {
        title[max_title_len] = '\0';  // Trunca si es muy largo
        printf("⚠️  Título truncado a %zu caracteres\n", max_title_len);
    }
    
    printf("⭐ Rating (1-5): "); 
    if (scanf("%f", &rating) != 1) {  // Intenta leer float
        printf("❌ Rating inválido\n");
        clear_input_buffer(); // Limpia buffer después de scanf
        return;
    }
    clear_input_buffer();
    
    if (rating < 1.0 || rating > 5.0) {
        printf("❌ Rating debe estar entre 1.0 y 5.0\n");
        return;
    }
    
    // // Prepara comando de forma segura
    int written = snprintf(shared->command, sizeof(shared->command), 
                          "search:%.*s:%.1f", 
                          (int)max_title_len, title, rating);
    
    if (written < 0 || written >= (int)sizeof(shared->command)) {
        printf("❌ Error creando comando de búsqueda\n");
        return;
    }
    
    shared->ready = 0; // Indica al motor que hay trabajo pendiente
    
    printf("\n⏳ Buscando...\n");
    
    // Espera resultados con timeout de 10 segundos
    int attempts = 0;
    int max_attempts = 100; // 100 intentos × 100ms = 10 segundos
    
    while (!shared->ready && attempts < max_attempts) {
        usleep(100000); // Espera 100ms (0.1 segundos)
        attempts++;
        
        // Muestra progreso cada 2 segundos
        if (attempts % 20 == 0) {
            printf("⏰ Todavía buscando... (%d segundos)\n", attempts / 10);
        }
    }
    
    if (shared->ready) {
        // Éxito: motor respondió a tiempo
        printf("\n📚 RESULTADOS:\n");
        printf("─────────────\n");
        printf("%s\n", shared->results); // Muestra resultados del motor
    } else {
        // Timeout: motor no respondió en 10 segundos
        printf("\n❌ TIMEOUT: El motor de búsqueda no respondió\n");
        strcpy(shared->results, "❌ Error: Timeout en la búsqueda");
    }
    
    // // Limpia para siguiente comando
    shared->command[0] = '\0';
    shared->results[0] = '\0';
}

int main() {
    printf("🔌 Conectando al motor de búsqueda...\n");
    
    // Conecta a memoria compartida
    key_t key = ftok(".", 'S');  // Crea clave única basada en directorio actual
    int shmid = shmget(key, SHM_SIZE, 0666);  // Obtiene segmento existente
    
    if (shmid == -1) {
        printf("❌ Error: Motor de búsqueda no encontrado\n");
        printf("   Ejecute primero: ./search_engine\n");
        return 1;
    }
    
    shared_data_t *shared = (shared_data_t*) shmat(shmid, NULL, 0);   // Conecta
    
    if (shared == (void*) -1) {
        printf("❌ Error: No se pudo acceder a la memoria compartida\n");
        return 1;
    }

    printf("✅ Conectado al motor de búsqueda\n");
    
    int choice;
    while (1) {
        display_menu();  // Muestra menú
        
        if (scanf("%d", &choice) != 1) {  // Lee opción
            printf("❌ Entrada inválida\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();  // Limpia buffer después de scanf
        
        switch (choice) {
            case 1:
                send_search_command(shared);  // Búsqueda
                break;
            case 2:
                printf("\n👋 Saliendo...\n");
                // Envía comando de salida al motor
                strcpy(shared->command, "exit");
                shared->ready = 0;
                shmdt(shared);  // Desconecta memoria compartida
                return 0;
            default:
                printf("❌ Opción inválida\n");
        }
        
        printf("\n⏎ Presione Enter para continuar...");
        getchar();  // Espera Enter para continuar
    }
    
    shmdt(shared);  // Desconecta memoria compartida
    return 0;
}