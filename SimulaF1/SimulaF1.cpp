// SIMULADOR DE CARRERA F1
// Programacion I - Proyecto Final
// Version con: neumaticos, pits, safety car, clima dinamico, combustible,
// radio, temporada, estadisticas historicas, personalidades IA y DRS.

#define _CRT_SECURE_NO_WARNINGS

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* CONSTANTES */
#define MAX_NOMBRE       12
#define MAX_AUTOS        16
#define ANCHO_VENTANA    1400
#define ALTO_VENTANA     900
#define FPS              60
#define ANCHO_PISTA      900
#define ALTO_CARRIL      48
#define MARGEN_X         80
#define MARGEN_Y         80
#define TIPO_DEPORTIVO   'D'
#define TIPO_TODOTERRENO 'T'
#define NITRO_MAX        35
#define PIT_TURNOS       3
#define FUEL_MAX         100.0f
#define NEUMATICOS_MAX   100.0f
#define PIT_MAX_PARADAS          5
#define PIT_VENTANA_DISTANCIA    350.0f
#define PIT_ANIMACION_TURNOS     18
#define AVERIA_TURNOS            4


/* Limpia la terminal en Windows */
#define LIMPIAR_CONSOLA() system("cls")

typedef enum { CLIMA_SECO, CLIMA_LLUVIA, CLIMA_NIEVE } Clima;
typedef enum {
    ESTADO_MENU, ESTADO_CORRIENDO, ESTADO_FIN,
    ESTADO_MARCAS, ESTADO_TEMPORADA, ESTADO_ESTADISTICAS
} EstadoJuego;
typedef enum { IA_AGRESIVO, IA_CONSERVADOR, IA_EQUILIBRADO } PersonalidadIA;

typedef union { float turbo; float traccion; } Especificaciones;

typedef struct {
    char nombre[MAX_NOMBRE];
    float velocidadBase, destreza;
    char tipoVehiculo;
    Especificaciones especificaciones;
    float posicion, velocidadActual;
    int accidentado, turnosAccidente, esJugador;
    int nitro, nitroUsado, adelantamientos, accidentesTotales;
    float velocidadMaxima;
    float neumaticos;
    int enPits, turnosPit, pitsRealizados, animacionPit, entrandoPits, saliendoPits;
    int penalizacionPitAplicada[PIT_MAX_PARADAS];
    float combustible;
    int eliminado, drsActivo;
    PersonalidadIA personalidad;
    ALLEGRO_COLOR color;
} Auto;

typedef struct { int longitudPista; Clima clima; int numAutos; Auto* autos; } Configuracion;

typedef struct {
    char nombre[MAX_NOMBRE];
    int turnos, accidentes, adelantamientos;
    float tiempo, velocidadMaxima;
} Marca;

typedef struct { char nombre[MAX_NOMBRE]; int puntos, victorias, carreras; } TemporadaPiloto;

typedef struct {
    char nombre[MAX_NOMBRE];
    int carreras, victorias, accidentes, adelantamientos;
    float mejorTiempo, velocidadMaxima;
} EstadisticaPiloto;

/* GLOBALES DE CARRERA */
static ALLEGRO_COLOR COLORES_AUTO[8];
static int safetyCarTurnos = 0;
static char ultimoEvento[160] = "Carrera lista para iniciar.";
static char radioEquipo[160] = "Ingeniero: Mantente concentrado.";
static int jugadorSeleccionado = 0;

/* PROTOTIPOS */
static void inicializarColores(void);
static const char* climaNombre(Clima c);
static const char* personalidadNombre(PersonalidadIA p);
static int ordenarPorPosicion(Configuracion* cfg, Auto* dest);
static const char* estadoAuto(Auto* a);
static int jugadorEnVentanaPit(Configuracion* cfg, Auto* a);
static int proximaParadaDisponible(Auto* a);
void formatearTiempo(float tiempo, char* buffer, int tam);
int leerConfiguracion(const char* archivo, Configuracion* cfg);
void liberarConfiguracion(Configuracion* cfg);
void limpiarBufferEntrada(void);
int leerFloatRango(const char* mensaje, float* valor, float minimo, float maximo);
int leerTipoVehiculo(char* tipo);

/* IMPLEMENTACIÓN DE COLORES */
static void inicializarColores(void) {
    COLORES_AUTO[0] = al_map_rgb(220, 20, 60);   // Rojo
    COLORES_AUTO[1] = al_map_rgb(30, 144, 255);  // Azul
    COLORES_AUTO[2] = al_map_rgb(50, 205, 50);   // Verde
    COLORES_AUTO[3] = al_map_rgb(255, 215, 0);   // Amarillo
    COLORES_AUTO[4] = al_map_rgb(255, 140, 0);   // Naranja
    COLORES_AUTO[5] = al_map_rgb(138, 43, 226);  // Morado
    COLORES_AUTO[6] = al_map_rgb(0, 255, 255);   // Cian
    COLORES_AUTO[7] = al_map_rgb(255, 255, 255); // Blanco
}

int agregarAutoManual(Configuracion* cfg)
{
    system("cls");
    printf("============================================================\n");
    printf("                    REGISTRAR NUEVO AUTO                    \n");
    printf("============================================================\n\n");

    if (cfg->numAutos >= MAX_AUTOS)
    {
        printf("No se pueden registrar mas autos.\n");
        printf("Limite maximo permitido: %d autos.\n", MAX_AUTOS);
        printf("\nPresiona ENTER para volver al menu...");
        getchar();
        system("cls");
        return 0;
    }

    Auto nuevo;
    memset(&nuevo, 0, sizeof(Auto));

    char tipo;
    float extra;
    int nombreValido = 0;

    do
    {
        // Se usa MAX_NOMBRE - 1 para reservar espacio al terminador nulo '\0'
        printf("Nombre del piloto (max %d caracteres): ", MAX_NOMBRE - 1);

        // scanf lee hasta (MAX_NOMBRE - 1) caracteres para no desbordar
  
        char formato[20];
        sprintf(formato, "%%%d[^\n]", MAX_NOMBRE - 1);

        if (scanf(formato, nuevo.nombre) != 1)
        {
            printf("Error: el nombre no puede estar vacio.\n");
            limpiarBufferEntrada();
            continue;
        }

        limpiarBufferEntrada();

        // Validar vacío
        if (strlen(nuevo.nombre) == 0)
        {
            printf("Error: el nombre no puede estar vacio.\n");
            continue;
        }

        // Validar longitud máxima
        if (strlen(nuevo.nombre) > (MAX_NOMBRE - 1))
        {
            printf("Error: el nombre debe tener maximo %d caracteres.\n", MAX_NOMBRE - 1); //Nos aseguramos que solo se registraron 11 caracteres
            continue;
        }

        nombreValido = 1;

        // Validar duplicados
        for (int i = 0; i < cfg->numAutos; i++)
        {
            if (strcmp(cfg->autos[i].nombre, nuevo.nombre) == 0)
            {
                printf("Error: ya existe un piloto con ese nombre.\n");
                nombreValido = 0;
                break;
            }
        }

    } while (!nombreValido);

    leerFloatRango("Velocidad base (50 - 300): ", &nuevo.velocidadBase, 50.0f, 300.0f);
    leerFloatRango("Destreza del piloto (0 - 100): ", &nuevo.destreza, 0.0f, 100.0f);

    leerTipoVehiculo(&tipo);
    nuevo.tipoVehiculo = tipo;

    if (tipo == TIPO_DEPORTIVO)
    {
        leerFloatRango("Turbo del auto (0 - 100): ", &extra, 0.0f, 100.0f);
        nuevo.especificaciones.turbo = extra;
    }
    else
    {
        leerFloatRango("Traccion del auto (0 - 100): ", &extra, 0.0f, 100.0f);
        nuevo.especificaciones.traccion = extra;
    }

    Auto* temp = (Auto*)realloc(cfg->autos, sizeof(Auto) * (cfg->numAutos + 1));

    // Nos aseguramos de que haya memoria
    if (!temp)
    {
        printf("\nNo se pudo reservar memoria para el nuevo auto.\n");
        printf("Presiona ENTER para continuar...");
        getchar();
        system("cls");
        return 0;
    }

    cfg->autos = temp;

    // Inicialización de estados
    nuevo.posicion = 0;
    nuevo.velocidadActual = 0;
    nuevo.accidentado = 0;
    nuevo.turnosAccidente = 0;
    nuevo.esJugador = 0;
    nuevo.nitro = 0;
    nuevo.nitroUsado = 0;
    nuevo.adelantamientos = 0;
    nuevo.accidentesTotales = 0;
    nuevo.velocidadMaxima = 0;
    nuevo.neumaticos = NEUMATICOS_MAX;
    nuevo.enPits = 0;
    nuevo.turnosPit = 0;
    nuevo.pitsRealizados = 0;
    nuevo.animacionPit = 0;
    nuevo.entrandoPits = 0;
    nuevo.saliendoPits = 0;

    for (int p = 0; p < PIT_MAX_PARADAS; p++)
    {
        nuevo.penalizacionPitAplicada[p] = 0;
    }

    nuevo.combustible = FUEL_MAX;
    nuevo.eliminado = 0;
    nuevo.drsActivo = 0;
    nuevo.personalidad = (PersonalidadIA)(cfg->numAutos % 3);
    nuevo.color = COLORES_AUTO[cfg->numAutos % 8];

    cfg->autos[cfg->numAutos] = nuevo;
    cfg->numAutos++;

    printf("\nAuto registrado correctamente.\n");
    printf("Ahora hay %d autos en la carrera.\n", cfg->numAutos);
    printf("\nPresiona ENTER para volver al menu...");
    getchar();

    system("cls");

    return 1;
}

static const char* climaNombre(Clima c)
{
    switch (c) {
    case CLIMA_LLUVIA: return "LLUVIA";
    case CLIMA_NIEVE:  return "NIEVE";
    default:           return "SECO";
    }
}

static const char* personalidadNombre(PersonalidadIA p)
{
    switch (p) {
    case IA_AGRESIVO:    return "Agresivo";
    case IA_CONSERVADOR: return "Conservador";
    default:             return "Equilibrado";
    }
}

void formatearTiempo(float tiempo, char* buffer, int tam)
{
    int minutos = (int)(tiempo / 60.0f);
    float segundos = tiempo - (minutos * 60.0f);
    snprintf(buffer, tam, "%02d:%06.3f", minutos, segundos);
}

/* MARCAS */
void guardarMarca(Auto ganador, int turnos, float tiempo)
{
    FILE* f = fopen("marcas.dat", "ab");
    if (!f) { printf("No se pudo abrir marcas.dat\n"); return; }

    Marca m;
    strcpy(m.nombre, ganador.nombre);
    m.turnos = turnos;
    m.tiempo = tiempo;
    m.velocidadMaxima = ganador.velocidadMaxima;
    m.accidentes = ganador.accidentesTotales;
    m.adelantamientos = ganador.adelantamientos;
    fwrite(&m, sizeof(Marca), 1, f);
    fclose(f);
}

void mostrarMejoresMarcas(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal)
{
    al_clear_to_color(al_map_rgb(10, 10, 25));
    al_draw_text(grande, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 50, ALLEGRO_ALIGN_CENTRE, "MEJORES MARCAS");

    FILE* f = fopen("marcas.dat", "rb");
    if (!f) {
        al_draw_text(normal, al_map_rgb(220, 220, 220), ANCHO_VENTANA / 2, 160, ALLEGRO_ALIGN_CENTRE, "Todavia no hay marcas registradas.");
        al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
        return;
    }

    Marca marcas[100];
    int total = 0;
    while (total < 100 && fread(&marcas[total], sizeof(Marca), 1, f) == 1) total++;
    fclose(f);

    for (int i = 0; i < total - 1; i++)
        for (int j = 0; j < total - 1 - i; j++)
            if (marcas[j].tiempo > marcas[j + 1].tiempo) {
                Marca temp = marcas[j]; marcas[j] = marcas[j + 1]; marcas[j + 1] = temp;
            }

    al_draw_text(normal, al_map_rgb(255, 200, 0), 120, 120, 0, "#");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 190, 120, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 410, 120, 0, "Turnos");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 540, 120, 0, "Tiempo");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 700, 120, 0, "Vel. Max");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 860, 120, 0, "Accidentes");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 1030, 120, 0, "Rebases");

    int limite = total < 10 ? total : 10;
    for (int i = 0; i < limite; i++) {
        char buf[128], tiempoTxt[32];
        int y = 160 + i * 35;
        formatearTiempo(marcas[i].tiempo, tiempoTxt, sizeof(tiempoTxt));

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 120, y, 0, buf);
        al_draw_text(normal, al_map_rgb(0, 220, 255), 190, y, 0, marcas[i].nombre);
        snprintf(buf, sizeof(buf), "%d", marcas[i].turnos);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 410, y, 0, buf);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 540, y, 0, tiempoTxt);
        snprintf(buf, sizeof(buf), "%.1f", marcas[i].velocidadMaxima);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 700, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", marcas[i].accidentes);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 860, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", marcas[i].adelantamientos);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 1030, y, 0, buf);
    }
    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
}

/* TEMPORADA */
static int buscarTemporadaPiloto(TemporadaPiloto* arr, int total, const char* nombre)
{
    for (int i = 0; i < total; i++)
        if (strcmp(arr[i].nombre, nombre) == 0) return i;
    return -1;
}

void guardarPuntosTemporada(Configuracion* cfg)
{
    TemporadaPiloto tabla[100];
    int total = 0;

    FILE* f = fopen("temporada.dat", "rb");
    if (f) {
        while (total < 100 && fread(&tabla[total], sizeof(TemporadaPiloto), 1, f) == 1) total++;
        fclose(f);
    }

    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);
    int puntosF1[10] = { 25, 18, 15, 12, 10, 8, 6, 4, 2, 1 };

    for (int i = 0; i < cnt && i < 10; i++) {
        int idx = buscarTemporadaPiloto(tabla, total, orden[i].nombre);
        if (idx < 0 && total < 100) {
            idx = total;
            strcpy(tabla[idx].nombre, orden[i].nombre);
            tabla[idx].puntos = tabla[idx].victorias = tabla[idx].carreras = 0;
            total++;
        }
        if (idx >= 0) {
            tabla[idx].puntos += puntosF1[i];
            tabla[idx].carreras++;
            if (i == 0) tabla[idx].victorias++;
        }
    }

    f = fopen("temporada.dat", "wb");
    if (f) { fwrite(tabla, sizeof(TemporadaPiloto), total, f); fclose(f); }
}

void mostrarTemporada(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal)
{
    al_clear_to_color(al_map_rgb(10, 10, 25));
    al_draw_text(grande, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 50, ALLEGRO_ALIGN_CENTRE, "TEMPORADA");

    FILE* f = fopen("temporada.dat", "rb");
    if (!f) {
        al_draw_text(normal, al_map_rgb(220, 220, 220), ANCHO_VENTANA / 2, 160, ALLEGRO_ALIGN_CENTRE, "Todavia no hay puntos de temporada.");
        al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
        return;
    }

    TemporadaPiloto tabla[100];
    int total = 0;
    while (total < 100 && fread(&tabla[total], sizeof(TemporadaPiloto), 1, f) == 1) total++;
    fclose(f);

    for (int i = 0; i < total - 1; i++)
        for (int j = 0; j < total - 1 - i; j++)
            if (tabla[j].puntos < tabla[j + 1].puntos) {
                TemporadaPiloto temp = tabla[j]; tabla[j] = tabla[j + 1]; tabla[j + 1] = temp;
            }

    al_draw_text(normal, al_map_rgb(255, 200, 0), 230, 130, 0, "POS");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 330, 130, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 620, 130, 0, "Puntos");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 780, 130, 0, "Victorias");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 980, 130, 0, "Carreras");

    int limite = total < 10 ? total : 10;
    for (int i = 0; i < limite; i++) {
        char buf[64];
        int y = 170 + i * 35;
        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 230, y, 0, buf);
        al_draw_text(normal, al_map_rgb(0, 220, 255), 330, y, 0, tabla[i].nombre);
        snprintf(buf, sizeof(buf), "%d", tabla[i].puntos);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 620, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", tabla[i].victorias);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 780, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", tabla[i].carreras);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 980, y, 0, buf);
    }
    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
}

/* ESTADISTICAS HISTORICAS */
static int buscarEstadisticaPiloto(EstadisticaPiloto* arr, int total, const char* nombre)
{
    for (int i = 0; i < total; i++)
        if (strcmp(arr[i].nombre, nombre) == 0) return i;
    return -1;
}

void guardarEstadisticasHistoricas(Configuracion* cfg, int ganadorIdx, float tiempoCarrera)
{
    EstadisticaPiloto tabla[100];
    int total = 0;

    FILE* f = fopen("estadisticas.dat", "rb");
    if (f) {
        while (total < 100 && fread(&tabla[total], sizeof(EstadisticaPiloto), 1, f) == 1) total++;
        fclose(f);
    }

    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        int idx = buscarEstadisticaPiloto(tabla, total, a->nombre);
        if (idx < 0 && total < 100) {
            idx = total;
            strcpy(tabla[idx].nombre, a->nombre);
            tabla[idx].carreras = tabla[idx].victorias = tabla[idx].accidentes = 0;
            tabla[idx].adelantamientos = 0;
            tabla[idx].mejorTiempo = tabla[idx].velocidadMaxima = 0;
            total++;
        }
        if (idx >= 0) {
            tabla[idx].carreras++;
            tabla[idx].accidentes += a->accidentesTotales;
            tabla[idx].adelantamientos += a->adelantamientos;
            if (a->velocidadMaxima > tabla[idx].velocidadMaxima)
                tabla[idx].velocidadMaxima = a->velocidadMaxima;
            if (i == ganadorIdx) {
                tabla[idx].victorias++;
                if (tabla[idx].mejorTiempo <= 0 || tiempoCarrera < tabla[idx].mejorTiempo)
                    tabla[idx].mejorTiempo = tiempoCarrera;
            }
        }
    }

    f = fopen("estadisticas.dat", "wb");
    if (f) { fwrite(tabla, sizeof(EstadisticaPiloto), total, f); fclose(f); }
}

void mostrarEstadisticas(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal)
{
    al_clear_to_color(al_map_rgb(10, 10, 25));
    al_draw_text(grande, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 50, ALLEGRO_ALIGN_CENTRE, "ESTADISTICAS HISTORICAS");

    FILE* f = fopen("estadisticas.dat", "rb");
    if (!f) {
        al_draw_text(normal, al_map_rgb(220, 220, 220), ANCHO_VENTANA / 2, 160, ALLEGRO_ALIGN_CENTRE, "Todavia no hay estadisticas historicas.");
        al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
        return;
    }

    EstadisticaPiloto tabla[100];
    int total = 0;
    while (total < 100 && fread(&tabla[total], sizeof(EstadisticaPiloto), 1, f) == 1) total++;
    fclose(f);

    for (int i = 0; i < total - 1; i++)
        for (int j = 0; j < total - 1 - i; j++)
            if (tabla[j].victorias < tabla[j + 1].victorias) {
                EstadisticaPiloto temp = tabla[j]; tabla[j] = tabla[j + 1]; tabla[j + 1] = temp;
            }

    al_draw_text(normal, al_map_rgb(255, 200, 0), 90, 130, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 310, 130, 0, "Carreras");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 470, 130, 0, "Victorias");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 640, 130, 0, "Acc.");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 770, 130, 0, "Rebases");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 940, 130, 0, "Mejor tiempo");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 1150, 130, 0, "VelMax");

    int limite = total < 10 ? total : 10;
    for (int i = 0; i < limite; i++) {
        char buf[64], tiempoTxt[32];
        int y = 170 + i * 35;
        formatearTiempo(tabla[i].mejorTiempo, tiempoTxt, sizeof(tiempoTxt));

        al_draw_text(normal, al_map_rgb(0, 220, 255), 90, y, 0, tabla[i].nombre);
        snprintf(buf, sizeof(buf), "%d", tabla[i].carreras);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 310, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", tabla[i].victorias);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 470, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", tabla[i].accidentes);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 640, y, 0, buf);
        snprintf(buf, sizeof(buf), "%d", tabla[i].adelantamientos);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 770, y, 0, buf);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 940, y, 0, tiempoTxt);
        snprintf(buf, sizeof(buf), "%.1f", tabla[i].velocidadMaxima);
        al_draw_text(normal, al_map_rgb(230, 230, 230), 1150, y, 0, buf);
    }
    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "Presiona ENTER para volver al menu");
}

/* CONFIGURACION */
int leerConfiguracion(const char* archivo, Configuracion* cfg)
{
    FILE* f = fopen(archivo, "r");
    if (!f) { fprintf(stderr, "Error: no se pudo abrir '%s'\n", archivo); return 0; }

    cfg->longitudPista = 500;
    cfg->clima = CLIMA_SECO;
    cfg->numAutos = 0;
    cfg->autos = NULL;

    char linea[256];
    int conteo = 0;
    while (fgets(linea, sizeof(linea), f))
        if (strncmp(linea, "auto=", 5) == 0) conteo++;
    rewind(f);

    cfg->autos = (Auto*)malloc(sizeof(Auto) * (conteo > 0 ? conteo : 1));
    if (!cfg->autos) { fclose(f); return 0; }

    int idx = 0;
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') continue;

        if (strncmp(linea, "longitud_pista=", 15) == 0) {
            cfg->longitudPista = atoi(linea + 15);
        }
        else if (strncmp(linea, "clima=", 6) == 0) {
            char cs[16];
            sscanf(linea + 6, "%15s", cs);
            if (strcmp(cs, "LLUVIA") == 0) cfg->clima = CLIMA_LLUVIA;
            else if (strcmp(cs, "NIEVE") == 0) cfg->clima = CLIMA_NIEVE;
            else                                cfg->clima = CLIMA_SECO;
        }
        else if (strncmp(linea, "auto=", 5) == 0 && idx < conteo) {
            Auto* a = &cfg->autos[idx];
            memset(a, 0, sizeof(Auto));
            char tipo;
            float extra;
            sscanf(linea + 5, "%31[^,],%f,%f,%c,%f", a->nombre, &a->velocidadBase, &a->destreza, &tipo, &extra);
            a->tipoVehiculo = tipo;
            if (tipo == TIPO_DEPORTIVO) a->especificaciones.turbo = extra;
            else                        a->especificaciones.traccion = extra;
            a->personalidad = (PersonalidadIA)(idx % 3);
            a->color = COLORES_AUTO[idx % 8];
            idx++;
        }
    }
    fclose(f);

    cfg->numAutos = idx;
    for (int i = 0; i < cfg->numAutos; i++) cfg->autos[i].esJugador = 0;
    if (cfg->numAutos > 0) cfg->autos[jugadorSeleccionado % cfg->numAutos].esJugador = 1;
    return 1;
}



void limpiarBufferEntrada(void)
{
    int c;

    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

int leerFloatRango(const char* mensaje, float* valor, float minimo, float maximo)
{
    int correcto = 0;

    do
    {
        printf("%s", mensaje);

        if (scanf("%f", valor) != 1)
        {
            printf("Error: debes escribir un numero valido.\n");
            limpiarBufferEntrada();
            continue;
        }

        limpiarBufferEntrada();

        if (*valor < minimo || *valor > maximo)
        {
            printf("Error: el valor debe estar entre %.1f y %.1f.\n", minimo, maximo);
            continue;
        }

        correcto = 1;

    } while (!correcto);

    return 1;
}

int leerTipoVehiculo(char* tipo)
{
    int correcto = 0;

    do
    {
        printf("Tipo de vehiculo (D = Deportivo, T = Todoterreno): ");

        if (scanf(" %c", tipo) != 1)
        {
            printf("Error: debes escribir D o T.\n");
            limpiarBufferEntrada();
            continue;
        }

        limpiarBufferEntrada();

        if (*tipo >= 'a' && *tipo <= 'z')
        {
            *tipo = *tipo - 32;
        }

        if (*tipo != TIPO_DEPORTIVO && *tipo != TIPO_TODOTERRENO)
        {
            printf("Error: tipo invalido. Solo se permite D o T.\n");
            continue;
        }

        correcto = 1;

    } while (!correcto);

    return 1;
}

static float factorClima(Clima clima, char tipo)
{
    if (clima == CLIMA_LLUVIA) return (tipo == TIPO_TODOTERRENO) ? 0.95f : 0.80f;
    if (clima == CLIMA_NIEVE)  return (tipo == TIPO_TODOTERRENO) ? 0.85f : 0.60f;
    return 1.0f;
}

/* NITRO DINAMICO */
void recargarNitroJugador(Configuracion* cfg, int turno)
{
    for (int i = 0; i < cfg->numAutos; i++) {
        if (!cfg->autos[i].esJugador) continue;
        int intervalo = (cfg->autos[i].nitro < 10) ? 6 : (cfg->autos[i].nitro < 25) ? 4 : 2;
        if (turno % intervalo == 0 && cfg->autos[i].nitro < NITRO_MAX) {
            cfg->autos[i].nitro++;
            if (cfg->autos[i].nitro > NITRO_MAX) cfg->autos[i].nitro = NITRO_MAX;
        }
        break;
    }
}

/* CLIMA DINAMICO */
void actualizarClimaDinamico(Configuracion* cfg, int turno)
{
    if (turno > 0 && turno % 70 == 0 && rand() % 100 < 35) {
        Clima anterior = cfg->clima;
        cfg->clima = (Clima)(rand() % 3);
        if (cfg->clima != anterior)
            snprintf(ultimoEvento, sizeof(ultimoEvento), "Cambio de clima: ahora hay %s.", climaNombre(cfg->clima));
    }
}

/* RADIO */
void actualizarRadio(Configuracion* cfg, int turno)
{
    if (turno % 25 != 0) return;

    Auto* jugador = NULL;
    for (int i = 0; i < cfg->numAutos; i++)
        if (cfg->autos[i].esJugador) { jugador = &cfg->autos[i]; break; }
    if (!jugador) return;

    if (jugador->neumaticos < 25)  strcpy(radioEquipo, "Ingeniero: Neumaticos muy gastados, considera entrar a pits.");
    else if (jugador->combustible < 20)  strcpy(radioEquipo, "Ingeniero: Combustible bajo, cuida el ritmo.");
    else if (jugador->drsActivo)         strcpy(radioEquipo, "Ingeniero: DRS disponible, aprovecha la recta.");
    else if (safetyCarTurnos > 0)        strcpy(radioEquipo, "Ingeniero: Safety Car en pista, conserva posicion.");
    else {
        const char* mensajes[] = {
            "Ingeniero: Buen ritmo, sigue asi.",
            "Ingeniero: Administra el nitro.",
            "Ingeniero: Mantente cerca para activar DRS.",
            "Ingeniero: Cuida las llantas en las curvas.",
            "Ingeniero: Tenemos buen ritmo de carrera."
        };
        strcpy(radioEquipo, mensajes[rand() % 5]);
    }
}

/* ---------------------------------------------------------------
   HELPERS DE PITS
--------------------------------------------------------------- */
float pitInicioVentana(Configuracion* cfg, int paradaIdx)
{
    float centro = cfg->longitudPista * (float)paradaIdx / (float)PIT_MAX_PARADAS;
    return centro - PIT_VENTANA_DISTANCIA;
}

float pitFinVentana(Configuracion* cfg, int paradaIdx)
{
    float centro = cfg->longitudPista * (float)paradaIdx / (float)PIT_MAX_PARADAS;
    return centro + PIT_VENTANA_DISTANCIA;
}

static int proximaParadaDisponible(Auto* a)
{
    for (int p = 1; p <= PIT_MAX_PARADAS; p++) {
        if (a->pitsRealizados >= p)          continue;
        if (a->penalizacionPitAplicada[p - 1]) continue;
        return p;
    }
    return 0;
}

static int jugadorEnVentanaPit(Configuracion* cfg, Auto* a)
{
    int siguiente = proximaParadaDisponible(a);
    if (siguiente == 0) return 0;
    return (a->posicion >= pitInicioVentana(cfg, siguiente) &&
        a->posicion <= pitFinVentana(cfg, siguiente));
}

void revisarPitsObligatorios(Configuracion* cfg)
{
    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        if (!a->esJugador) continue;
        for (int p = 1; p <= PIT_MAX_PARADAS; p++) {
            if (a->pitsRealizados >= p)          continue;
            if (a->penalizacionPitAplicada[p - 1]) continue;
            if (a->posicion > pitFinVentana(cfg, p)) {
                a->penalizacionPitAplicada[p - 1] = 1;
                a->accidentado = 1;
                a->turnosAccidente = AVERIA_TURNOS;
                a->accidentesTotales++;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "Averia: %s perdio la ventana de pits #%d.", a->nombre, p);
                strcpy(radioEquipo, "Ingeniero: Perdimos la ventana. Averia en el auto.");
                break;
            }
        }
        break;
    }
}

/* ACTUALIZACION PRINCIPAL */
void actualizarPosiciones(Configuracion* cfg, int teclaArriba, int teclaAbajo, int teclaPit, int turno)
{
    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        float posicionAnterior = a->posicion;

        if (a->eliminado) { a->velocidadActual = 0; continue; }

        if (a->enPits) {
            a->turnosPit--;
            a->velocidadActual = 0;
            if (a->animacionPit > 0) a->animacionPit--;
            if (a->turnosPit <= 0) {
                a->enPits = 0;
                a->saliendoPits = 1;
                a->entrandoPits = 0;
                a->animacionPit = PIT_ANIMACION_TURNOS;
                a->neumaticos = NEUMATICOS_MAX;
                a->combustible = FUEL_MAX;
                a->nitro = NITRO_MAX;
                if (a->esJugador && a->pitsRealizados < PIT_MAX_PARADAS) a->pitsRealizados++;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "%s salio de pits con neumaticos nuevos.", a->nombre);
            }
            continue;
        }

        if (a->animacionPit > 0) a->animacionPit--;
        else { a->entrandoPits = 0; a->saliendoPits = 0; }

        if (a->accidentado) {
            if (--a->turnosAccidente <= 0) a->accidentado = 0;
            a->velocidadActual = 0;
            continue;
        }

        if (a->combustible <= 0) {
            a->eliminado = 1;
            a->velocidadActual = 0;
            snprintf(ultimoEvento, sizeof(ultimoEvento), "%s abandono por falta de combustible.", a->nombre);
            continue;
        }

        float vel = a->velocidadBase;
        if (a->tipoVehiculo == TIPO_DEPORTIVO) vel += a->especificaciones.turbo * 0.05f;
        else                                   vel += a->especificaciones.traccion * 0.03f;

        int usandoNitro = 0;

        /* JUGADOR */
        if (a->esJugador) {
            if (teclaPit && !a->enPits) {
                if (proximaParadaDisponible(a) == 0) {
                    strcpy(radioEquipo, "Ingeniero: Ya usamos todas las paradas disponibles.");
                }
                else if (jugadorEnVentanaPit(cfg, a)) {
                    int sig = proximaParadaDisponible(a);
                    a->enPits = 1; a->turnosPit = PIT_TURNOS;
                    a->entrandoPits = 1; a->saliendoPits = 0;
                    a->animacionPit = PIT_ANIMACION_TURNOS;
                    snprintf(ultimoEvento, sizeof(ultimoEvento), "%s entro a pits. Parada %d de %d.", a->nombre, sig, PIT_MAX_PARADAS);
                    strcpy(radioEquipo, "Ingeniero: Entrando a pits. Cambio de neumaticos en proceso.");
                    continue;
                }
                else {
                    int siguiente = proximaParadaDisponible(a);
                    snprintf(radioEquipo, sizeof(radioEquipo),
                        "Ingeniero: Fuera de ventana. Pit #%d: %.0f - %.0f unidades.",
                        siguiente, pitInicioVentana(cfg, siguiente), pitFinVentana(cfg, siguiente));
                }
            }

            if (teclaArriba && a->nitro > 0) { usandoNitro = 1; a->nitro--; a->nitroUsado++; }
            if (teclaAbajo) vel *= 0.70f;

            float lider = 0.0f;
            for (int j = 0; j < cfg->numAutos; j++)
                if (cfg->autos[j].posicion > lider) lider = cfg->autos[j].posicion;

            float diferenciaLider = lider - a->posicion;
            if (diferenciaLider > 800) vel *= 1.18f;
            else if (diferenciaLider > 400) vel *= 1.10f;
        }
        /* IA */
        else {
            if (a->neumaticos < 25 && !a->enPits && rand() % 100 < 25) {
                a->enPits = 1; a->turnosPit = PIT_TURNOS;
                a->entrandoPits = 1; a->saliendoPits = 0;
                a->animacionPit = PIT_ANIMACION_TURNOS;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "%s entro a pits por desgaste.", a->nombre);
                continue;
            }

            float lider = 0.0f, masCercanoAdelante = 999999.0f;
            int vaUltimo = 1;
            for (int j = 0; j < cfg->numAutos; j++) {
                if (cfg->autos[j].posicion > lider) lider = cfg->autos[j].posicion;
                if (cfg->autos[j].posicion < a->posicion) vaUltimo = 0;
                float dif = cfg->autos[j].posicion - a->posicion;
                if (dif > 0 && dif < masCercanoAdelante) masCercanoAdelante = dif;
            }

            float diferenciaLider = lider - a->posicion;
            if (diferenciaLider > 400) vel *= 1.30f;
            else if (diferenciaLider > 200) vel *= 1.18f;
            else if (diferenciaLider > 100) vel *= 1.10f;

            if (vaUltimo)               vel *= 1.12f;
            if (masCercanoAdelante < 90) vel *= 1.08f;
            if (masCercanoAdelante < 35) vel *= 1.12f;

            if (a->personalidad == IA_AGRESIVO)    vel *= 1.08f;
            else if (a->personalidad == IA_CONSERVADOR) vel *= 0.97f;

            vel *= (1.0f + ((float)rand() / RAND_MAX) * 0.18f - 0.04f);
        }

        /* DRS */
        a->drsActivo = 0;
        for (int j = 0; j < cfg->numAutos; j++) {
            if (i == j) continue;
            float dif = cfg->autos[j].posicion - a->posicion;
            if (dif > 0 && dif < 100 && safetyCarTurnos == 0) { a->drsActivo = 1; vel *= 1.10f; break; }
        }

        /* CLIMA */
        vel *= factorClima(cfg->clima, a->tipoVehiculo);

        /* NEUMATICOS */
        if (a->neumaticos < 20) vel *= 0.70f;
        else if (a->neumaticos < 40) vel *= 0.85f;
        else if (a->neumaticos < 70) vel *= 0.95f;

        /* COMBUSTIBLE */
        if (a->combustible < 15) vel *= 0.85f;

        /* SAFETY CAR */
        if (safetyCarTurnos > 0) vel *= 0.45f;

        /* NITRO */
        if (a->esJugador && usandoNitro && safetyCarTurnos == 0) { vel *= 1.60f; vel += 20.0f; }

        /* EVENTOS */
        if (safetyCarTurnos == 0 && rand() % 100 < 7) {
            int evento = rand() % 3;
            if (evento == 0) {
                vel *= 1.18f;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "%s encontro una recta perfecta.", a->nombre);
            }
            else if (evento == 1) {
                vel *= 0.85f;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "%s perdio velocidad en una curva.", a->nombre);
            }
            else {
                vel *= 1.08f;
                snprintf(ultimoEvento, sizeof(ultimoEvento), "%s intento un rebase agresivo.", a->nombre);
            }
        }

        if (vel < 5.0f) vel = 5.0f;
        a->velocidadActual = vel;
        a->posicion += vel;
        if (a->velocidadActual > a->velocidadMaxima) a->velocidadMaxima = a->velocidadActual;

        /* DESGASTE */
        float desgaste = 0.35f;
        if (a->drsActivo)                            desgaste += 0.08f;
        if (usandoNitro)                             desgaste += 0.15f;
        if (a->personalidad == IA_AGRESIVO)          desgaste += 0.08f;
        if (cfg->clima == CLIMA_LLUVIA)              desgaste += 0.05f;
        if (cfg->clima == CLIMA_NIEVE)               desgaste += 0.10f;
        a->neumaticos -= desgaste;
        if (a->neumaticos < 0) a->neumaticos = 0;

        a->combustible -= 0.25f;
        if (usandoNitro) a->combustible -= 0.10f;
        if (a->combustible < 0) a->combustible = 0;

        /* ACCIDENTES */
        float prob = (100.0f - a->destreza) / 1200.0f;
        if (a->esJugador && usandoNitro)                       prob *= 1.5f;
        if (!a->esJugador && a->personalidad == IA_AGRESIVO)   prob *= 1.4f;
        if (a->neumaticos < 20)                                 prob *= 1.8f;
        if (cfg->clima == CLIMA_LLUVIA)                         prob *= 1.6f;
        if (cfg->clima == CLIMA_NIEVE)                          prob *= 2.4f;

        if (safetyCarTurnos == 0 && (float)rand() / RAND_MAX < prob) {
            a->accidentado = 1;
            a->turnosAccidente = 2 + rand() % 3;
            a->accidentesTotales++;
            safetyCarTurnos = 3;
            snprintf(ultimoEvento, sizeof(ultimoEvento), "Accidente de %s. Safety Car en pista.", a->nombre);
        }

        /* REBASES */
        for (int j = 0; j < cfg->numAutos; j++) {
            if (i == j) continue;
            if (posicionAnterior < cfg->autos[j].posicion && a->posicion > cfg->autos[j].posicion)
                a->adelantamientos++;
        }
    }

    if (safetyCarTurnos > 0) safetyCarTurnos--;
}

/* GANADOR */
int determinarGanador(Configuracion* cfg)
{
    for (int i = 0; i < cfg->numAutos; i++)
        if (!cfg->autos[i].eliminado && cfg->autos[i].posicion >= cfg->longitudPista) return i;
    return -1;
}

void liberarConfiguracion(Configuracion* cfg)
{
    if (cfg->autos) { free(cfg->autos); cfg->autos = NULL; }
    cfg->numAutos = 0;
}

/* HELPERS REUTILIZABLES */
static int ordenarPorPosicion(Configuracion* cfg, Auto* dest)
{
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;
    for (int i = 0; i < cnt; i++) dest[i] = cfg->autos[i];
    for (int i = 0; i < cnt - 1; i++)
        for (int j = 0; j < cnt - 1 - i; j++)
            if (dest[j].posicion < dest[j + 1].posicion) {
                Auto t = dest[j]; dest[j] = dest[j + 1]; dest[j + 1] = t;
            }
    return cnt;
}

static const char* estadoAuto(Auto* a)
{
    if (a->eliminado)   return "OUT";
    if (a->enPits)      return "PITS";
    if (a->accidentado) return "CRASH";
    if (a->drsActivo)   return "DRS";
    return "OK";
}

void imprimirConsolaFinal(Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));
    LIMPIAR_CONSOLA();

    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);

    printf("==========================================================================\n");
    printf("                            BANDERA A CUADROS                             \n");
    printf("==========================================================================\n\n");
    if (ganadorIdx >= 0) printf(" GANADOR: %s\n", cfg->autos[ganadorIdx].nombre);
    printf(" TURNOS : %d\n TIEMPO : %s\n\n", turno, tiempoTxt);
    printf(" PODIO\n");
    printf("--------------------------------------------------------------------------\n");
    if (cnt >= 1) printf(" 1. %s\n", orden[0].nombre);
    if (cnt >= 2) printf(" 2. %s\n", orden[1].nombre);
    if (cnt >= 3) printf(" 3. %s\n", orden[2].nombre);

    printf("\n CLASIFICACION FINAL\n");
    printf("--------------------------------------------------------------------------\n");
    printf(" %-3s | %-14s | %-10s | %-8s | %-8s | %-8s\n", "POS", "Piloto", "Distancia", "VelMax", "Rebases", "Choques");
    for (int i = 0; i < cnt; i++)
        printf(" %-3d | %-14s | %10.1f | %8.1f | %8d | %8d\n",
            i + 1, orden[i].nombre, orden[i].posicion,
            orden[i].velocidadMaxima, orden[i].adelantamientos, orden[i].accidentesTotales);
    printf("==========================================================================\n");
}

void imprimirConsolaEstado(Configuracion* cfg, int turno, float tiempoCarrera)
{
    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));
    LIMPIAR_CONSOLA();

    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);
    float posicionLider = orden[0].posicion;

    printf("==========================================================================\n");
    printf("                         SIMULADOR DE CARRERA F1                         \n");
    printf("==========================================================================\n");
    printf(" Turno: %-5d | Tiempo: %-10s | Clima: %-8s | Pista: %d unidades\n",
        turno, tiempoTxt, climaNombre(cfg->clima), cfg->longitudPista);
    if (safetyCarTurnos > 0) printf(" SAFETY CAR: %d turnos restantes\n", safetyCarTurnos);
    printf(" Evento: %s\n Radio : %s\n", ultimoEvento, radioEquipo);
    printf("--------------------------------------------------------------------------\n");
    printf(" %-3s | %-12s | %-11s | %-6s | %-7s | %-7s | %-7s | %-6s\n",
        "POS", "Piloto", "Distancia", "Vel", "Gap", "Neum", "Fuel", "Estado");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < cnt; i++) {
        Auto* a = &orden[i];
        char gap[32];
        if (i == 0) strcpy(gap, "Lider");
        else snprintf(gap, sizeof(gap), "+%.0f", posicionLider - a->posicion);
        printf(" %-3d | %-12s | %6.0f/%-4d | %6.1f | %-7s | %5.1f%% | %5.1f%% | %-6s\n",
            i + 1, a->nombre, a->posicion, cfg->longitudPista,
            a->velocidadActual, gap, a->neumaticos, a->combustible, estadoAuto(a));
    }
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < cfg->numAutos; i++) {
        if (!cfg->autos[i].esJugador) continue;
        Auto* j = &cfg->autos[i];

        float progreso = j->posicion / cfg->longitudPista * 100.0f;
        if (progreso > 100.0f) progreso = 100.0f;
        int bp = (int)(progreso / 5.0f);
        int bn = (j->nitro * 20) / NITRO_MAX;
        if (bn > 20) bn = 20;

        printf(" Tu auto: %s\n", j->nombre);
        printf(" Progreso: ["); for (int b = 0; b < 20; b++) printf(b < bp ? "#" : "-"); printf("] %.1f%%\n", progreso);
        printf(" Nitro:    ["); for (int b = 0; b < 20; b++) printf(b < bn ? "#" : "-"); printf("] %d/%d\n", j->nitro, NITRO_MAX);
        printf(" Neumaticos: %.1f%% | Combustible: %.1f%% | DRS: %s\n",
            j->neumaticos, j->combustible, j->drsActivo ? "ACTIVO" : "NO");

        if (j->pitsRealizados < PIT_MAX_PARADAS) {
            int sig = proximaParadaDisponible(j);
            if (sig == 0)
                printf(" Pits: %d/%d | Todas las ventanas cubiertas\n", j->pitsRealizados, PIT_MAX_PARADAS);
            else
                printf(" Pits: %d/%d | Ventana #%d: %.0f - %.0f unidades%s\n",
                    j->pitsRealizados, PIT_MAX_PARADAS, sig,
                    pitInicioVentana(cfg, sig), pitFinVentana(cfg, sig),
                    jugadorEnVentanaPit(cfg, j) ? "  <-- ENTRAR AHORA (P)" : "");
        }
        else {
            printf(" Pits: %d/%d | Sin paradas disponibles\n", j->pitsRealizados, PIT_MAX_PARADAS);
        }

        printf(" Nitro usado: %d | Rebases: %d | Accidentes: %d | Vel.Max: %.1f\n",
            j->nitroUsado, j->adelantamientos, j->accidentesTotales, j->velocidadMaxima);
        break;
    }
    printf("==========================================================================\n");
    printf(" Arriba=NITRO  Abajo=Frenar  P=Pit (solo en ventana)  ESC=Salir\n");
    printf("==========================================================================\n");
    fflush(stdout);
}

/* GRAFICOS */
static void dibujarAuto(float cx, float cy, ALLEGRO_COLOR color,
    int esJugador, int accidentado, const char* inicial, ALLEGRO_FONT* fuente)
{
    ALLEGRO_COLOR c = accidentado ? al_map_rgb(255, 80, 80) : color;
    float w = 36, h = 18;

    al_draw_filled_rounded_rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2, 5, 5, c);
    al_draw_filled_rounded_rectangle(cx - 8, cy - h / 2 - 6, cx + 8, cy - h / 2 + 2, 3, 3, al_map_rgba(255, 255, 255, 120));

    ALLEGRO_COLOR rueda = al_map_rgb(30, 30, 30);
    al_draw_filled_circle(cx - w / 2 + 5, cy + h / 2 - 2, 5, rueda);
    al_draw_filled_circle(cx + w / 2 - 5, cy + h / 2 - 2, 5, rueda);

    if (esJugador)
        al_draw_rounded_rectangle(cx - w / 2 - 2, cy - h / 2 - 8, cx + w / 2 + 2, cy + h / 2 + 2, 6, 6, al_map_rgb(0, 220, 255), 2.0f);

    if (fuente)
        al_draw_text(fuente, al_map_rgb(10, 10, 10), cx, cy - 6, ALLEGRO_ALIGN_CENTRE, inicial);

    if (accidentado && fuente)
        al_draw_text(fuente, al_map_rgb(255, 80, 80), cx, cy - h / 2 - 20, ALLEGRO_ALIGN_CENTRE, "X");
}

void dibujarPista(Configuracion* cfg, ALLEGRO_FONT* fuente, int turno, float tiempoCarrera)
{
    int n = cfg->numAutos;
    float escala = 2.0f, camara = 0.0f;

    for (int j = 0; j < n; j++) {
        if (cfg->autos[j].esJugador) {
            camara = cfg->autos[j].posicion - 250.0f;
            if (camara < 0) camara = 0;
            break;
        }
    }

    al_draw_filled_rectangle(0, MARGEN_Y - 20, ANCHO_VENTANA, MARGEN_Y + n * ALTO_CARRIL + 20, al_map_rgb(30, 80, 30));

    for (int i = 0; i < n; i++) {
        float y0 = MARGEN_Y + i * ALTO_CARRIL;
        ALLEGRO_COLOR col = (i % 2 == 0) ? al_map_rgb(50, 50, 65) : al_map_rgb(42, 42, 55);
        al_draw_filled_rectangle(MARGEN_X, y0, MARGEN_X + ANCHO_PISTA, y0 + ALTO_CARRIL, col);
        for (int x = MARGEN_X - ((int)(camara * escala) % 40); x < MARGEN_X + ANCHO_PISTA; x += 40)
            al_draw_filled_rectangle(x, y0 + ALTO_CARRIL / 2 - 1, x + 20, y0 + ALTO_CARRIL / 2 + 1, al_map_rgba(200, 200, 200, 60));
    }

    float py0 = MARGEN_Y, py1 = MARGEN_Y + n * ALTO_CARRIL;
    al_draw_rectangle(MARGEN_X, py0, MARGEN_X + ANCHO_PISTA, py1, al_map_rgb(200, 180, 50), 3.0f);

    /* Taller y pit lane */
    float pitY = py1 + 12;
    al_draw_filled_rounded_rectangle(MARGEN_X, pitY, MARGEN_X + ANCHO_PISTA, pitY + 54, 8, 8, al_map_rgb(45, 38, 32));
    al_draw_rounded_rectangle(MARGEN_X, pitY, MARGEN_X + ANCHO_PISTA, pitY + 54, 8, 8, al_map_rgb(255, 200, 0), 2.0f);

    for (int t = 0; t < 5; t++) {
        float bx = MARGEN_X + 120 + t * 140;
        al_draw_filled_rectangle(bx, pitY + 8, bx + 95, pitY + 48, al_map_rgb(70, 70, 85));
        al_draw_rectangle(bx, pitY + 8, bx + 95, pitY + 48, al_map_rgb(180, 180, 190), 1.0f);
        al_draw_filled_circle(bx + 18, pitY + 26, 9, al_map_rgb(20, 20, 20));
        al_draw_filled_circle(bx + 77, pitY + 26, 9, al_map_rgb(20, 20, 20));
        al_draw_filled_rectangle(bx + 40, pitY + 14, bx + 55, pitY + 42, al_map_rgb(255, 200, 0));
        if (fuente) {
            char pitLabel[16];
            snprintf(pitLabel, sizeof(pitLabel), "BOX %d", t + 1);
            al_draw_text(fuente, al_map_rgb(255, 255, 255), bx + 47, pitY + 18, ALLEGRO_ALIGN_CENTRE, pitLabel);
        }
    }
    if (fuente) al_draw_text(fuente, al_map_rgb(255, 200, 0), MARGEN_X + 10, pitY + 60, 0, "TALLER / PIT LANE");

    float xMeta = MARGEN_X + (cfg->longitudPista - camara) * escala;
    if (xMeta >= MARGEN_X && xMeta <= MARGEN_X + ANCHO_PISTA) {
        for (int k = 0; k < (int)(py1 - py0); k += 10) {
            ALLEGRO_COLOR mc = (k / 10 % 2 == 0) ? al_map_rgb(255, 255, 255) : al_map_rgb(0, 0, 0);
            al_draw_filled_rectangle(xMeta - 5, py0 + k, xMeta + 5, py0 + k + 10, mc);
        }
        if (fuente) al_draw_text(fuente, al_map_rgb(255, 255, 100), xMeta, py0 - 18, ALLEGRO_ALIGN_CENTRE, "META");
    }
    if (fuente) al_draw_text(fuente, al_map_rgb(100, 200, 100), MARGEN_X - 30, py0 + 5, ALLEGRO_ALIGN_CENTRE, "SALIDA");

    for (int i = 0; i < n; i++) {
        Auto* a = &cfg->autos[i];
        float px = MARGEN_X + (a->posicion - camara) * escala;
        float py = MARGEN_Y + i * ALTO_CARRIL + ALTO_CARRIL / 2.0f;

        if (a->enPits || a->entrandoPits || a->saliendoPits) {
            float avanceAnim = 1.0f - (a->animacionPit / (float)PIT_ANIMACION_TURNOS);
            if (avanceAnim < 0.0f) avanceAnim = 0.0f;
            if (avanceAnim > 1.0f) avanceAnim = 1.0f;

            float boxX = MARGEN_X + 165 + (i % 5) * 140;
            float boxY = MARGEN_Y + n * ALTO_CARRIL + 38;

            if (a->entrandoPits) { px = px + (boxX - px) * avanceAnim; py = py + (boxY - py) * avanceAnim; }
            else if (a->saliendoPits) { px = boxX + (px - boxX) * avanceAnim; py = boxY + (py - boxY) * avanceAnim; }
            else { px = boxX; py = boxY; }
        }

        if (px >= MARGEN_X - 40 && px <= MARGEN_X + ANCHO_PISTA + 40) {
            char ini[3] = { a->nombre[0], '\0' };
            dibujarAuto(px, py, a->color, a->esJugador, a->accidentado, ini, fuente);
            if (fuente && (a->enPits || a->entrandoPits || a->saliendoPits))
                al_draw_text(fuente, al_map_rgb(255, 200, 0), px, py - 35, ALLEGRO_ALIGN_CENTRE,
                    a->enPits ? "PITS" : (a->entrandoPits ? "ENTRANDO" : "SALIENDO"));
        }
    }

    if (fuente) {
        char buf[160], tiempoTxt[32];
        formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

        snprintf(buf, sizeof(buf), "Turno: %d | Tiempo: %s", turno, tiempoTxt);
        al_draw_text(fuente, al_map_rgb(230, 230, 230), 10, 10, 0, buf);

        snprintf(buf, sizeof(buf), "Clima: %s | Safety Car: %s", climaNombre(cfg->clima), safetyCarTurnos > 0 ? "SI" : "NO");
        al_draw_text(fuente, al_map_rgb(255, 200, 0), 10, 28, 0, buf);

        for (int i = 0; i < cfg->numAutos; i++) {
            if (!cfg->autos[i].esJugador) continue;
            Auto* jug = &cfg->autos[i];

            if (jug->pitsRealizados < PIT_MAX_PARADAS) {
                int sig = proximaParadaDisponible(jug);
                if (sig == 0)
                    snprintf(buf, sizeof(buf), "Nitro:%d | Neum:%.0f%% | Fuel:%.0f%% | DRS:%s | Pits completos",
                        jug->nitro, jug->neumaticos, jug->combustible, jug->drsActivo ? "ON" : "OFF");
                else {
                    int enVentana = jugadorEnVentanaPit(cfg, jug);
                    snprintf(buf, sizeof(buf),
                        "Nitro:%d | Neum:%.0f%% | Fuel:%.0f%% | DRS:%s | Pits:%d/%d | Ventana#%d: %.0f-%.0f%s",
                        jug->nitro, jug->neumaticos, jug->combustible, jug->drsActivo ? "ON" : "OFF",
                        jug->pitsRealizados, PIT_MAX_PARADAS, sig,
                        pitInicioVentana(cfg, sig), pitFinVentana(cfg, sig), enVentana ? "  <P>" : "");
                }
            }
            else {
                snprintf(buf, sizeof(buf), "Nitro:%d | Neum:%.0f%% | Fuel:%.0f%% | DRS:%s | Pits:%d/%d",
                    jug->nitro, jug->neumaticos, jug->combustible, jug->drsActivo ? "ON" : "OFF",
                    jug->pitsRealizados, PIT_MAX_PARADAS);
            }
            al_draw_text(fuente, al_map_rgb(0, 220, 255), 10, 64, 0, buf);
            break;
        }

        snprintf(buf, sizeof(buf), "Camara: %.0f / %d", camara, cfg->longitudPista);
        al_draw_text(fuente, al_map_rgb(200, 200, 200), 10, 46, 0, buf);
        al_draw_text(fuente, al_map_rgb(0, 220, 255), ANCHO_VENTANA - 10, 10, ALLEGRO_ALIGN_RIGHT, "Borde cian = TU auto");
        al_draw_text(fuente, al_map_rgb(200, 200, 200), ANCHO_VENTANA - 10, 28, ALLEGRO_ALIGN_RIGHT, "Arriba=NITRO  Abajo=Frenar  P=Pits  ESC=Salir");

        al_draw_filled_rounded_rectangle(330, ALTO_VENTANA - 120, 1070, ALTO_VENTANA - 58, 12, 12, al_map_rgba(10, 10, 25, 210));
        al_draw_rounded_rectangle(330, ALTO_VENTANA - 120, 1070, ALTO_VENTANA - 58, 12, 12, al_map_rgb(80, 180, 255), 2.0f);
        al_draw_text(fuente, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, ALTO_VENTANA - 112, ALLEGRO_ALIGN_CENTRE, ultimoEvento);
        al_draw_text(fuente, al_map_rgb(230, 230, 230), ANCHO_VENTANA / 2, ALTO_VENTANA - 88, ALLEGRO_ALIGN_CENTRE, radioEquipo);
    }
}

void dibujarTabla(Configuracion* cfg, ALLEGRO_FONT* fuente)
{
    if (!fuente) return;

    int n = cfg->numAutos;
    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);

    float xs = MARGEN_X + ANCHO_PISTA + 30;
    float ty = MARGEN_Y;

    // Aumenté el ancho de la columna de posición (cw[2]) de 75 a 90 
    // para que "1000/1000" quepa sin empujar a la velocidad.
    float cw[6] = { 25, 110, 90, 60, 60, 50 };
    const char* hdr[] = { "#", "Piloto", "Pos.", "Vel.", "Neum.", "Est." };

    float altoFondo = 30 + (cnt * 20);
    al_draw_filled_rounded_rectangle(xs - 10, ty - 10, xs + 390, ty + altoFondo, 10, 10, al_map_rgba(15, 15, 30, 230));
    al_draw_rounded_rectangle(xs - 10, ty - 10, xs + 390, ty + altoFondo, 10, 10, al_map_rgb(0, 150, 220), 2.0f);

    float x = xs;
    for (int c = 0; c < 6; c++) {
        al_draw_text(fuente, al_map_rgb(255, 200, 0), x, ty, 0, hdr[c]);
        x += cw[c];
    }
    ty += 18;
    al_draw_line(xs, ty, xs + 380, ty, al_map_rgb(100, 150, 200), 1.0f);
    ty += 6;

    for (int i = 0; i < cnt; i++) {
        Auto* a = &orden[i];
        char buf[32];
        x = xs;
        ALLEGRO_COLOR rc = a->esJugador ? al_map_rgb(0, 220, 255)
            : a->accidentado ? al_map_rgb(255, 80, 80)
            : al_map_rgb(230, 230, 230);

        // 1. Número
        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(fuente, rc, x, ty, 0, buf); x += cw[0];

        // 2. Nombre (Truncamos a 15 caracteres visuales para que no invada columnas)
        char nombreCorto[16];
        snprintf(nombreCorto, sizeof(nombreCorto), "%.15s", a->nombre);
        al_draw_text(fuente, rc, x, ty, 0, nombreCorto); x += cw[1];

        // 3. Posición (Fijo para que no empuje)
        snprintf(buf, sizeof(buf), "%.0f/%d", a->posicion, cfg->longitudPista);
        al_draw_text(fuente, rc, x, ty, 0, buf); x += cw[2];

        // 4. Velocidad
        snprintf(buf, sizeof(buf), "%.0f", a->velocidadActual);
        al_draw_text(fuente, rc, x, ty, 0, buf); x += cw[3];

        // 5. Neumáticos
        snprintf(buf, sizeof(buf), "%.0f%%", a->neumaticos);
        al_draw_text(fuente, rc, x, ty, 0, buf); x += cw[4];

        // 6. Estado
        al_draw_text(fuente, rc, x, ty, 0, estadoAuto(a));

        ty += 20;
    }
}

static void dibujarMenu(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal, Configuracion* cfg)
{
    al_clear_to_color(al_map_rgb(8, 10, 25));
    al_draw_filled_rectangle(0, 0, ANCHO_VENTANA, ALTO_VENTANA / 2, al_map_rgb(8, 10, 25));
    al_draw_filled_rectangle(0, ALTO_VENTANA / 2, ANCHO_VENTANA, ALTO_VENTANA, al_map_rgb(28, 25, 60));

    al_draw_filled_rounded_rectangle(250, 40, 1150, 150, 20, 20, al_map_rgb(20, 25, 55));
    al_draw_rounded_rectangle(250, 40, 1150, 150, 20, 20, al_map_rgb(255, 200, 0), 3);
    al_draw_text(grande, al_map_rgb(255, 210, 40), ANCHO_VENTANA / 2, 65, ALLEGRO_ALIGN_CENTRE, "SIMULADOR DE CARRERA F1");
    al_draw_text(normal, al_map_rgb(210, 210, 210), ANCHO_VENTANA / 2, 115, ALLEGRO_ALIGN_CENTRE, "Proyecto Final - Programacion I");

    char buf[160];
    al_draw_filled_rounded_rectangle(280, 185, 1120, 265, 15, 15, al_map_rgb(18, 22, 45));
    al_draw_rounded_rectangle(280, 185, 1120, 265, 15, 15, al_map_rgb(80, 180, 255), 2);

    snprintf(buf, sizeof(buf), "Pista: %d unidades", cfg->longitudPista);
    al_draw_text(normal, al_map_rgb(230, 230, 230), 330, 205, 0, buf);
    snprintf(buf, sizeof(buf), "Clima: %s", climaNombre(cfg->clima));
    al_draw_text(normal, al_map_rgb(255, 200, 0), 600, 205, 0, buf);
    snprintf(buf, sizeof(buf), "Autos registrados: %d", cfg->numAutos);
    al_draw_text(normal, al_map_rgb(0, 220, 255), 820, 205, 0, buf);
    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, 238, ALLEGRO_ALIGN_CENTRE,
        "ENTER iniciar | A agregar auto | M marcas | T temporada | H historico | ESC salir");

    al_draw_text(normal, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 300, ALLEGRO_ALIGN_CENTRE,
        "PILOTOS REGISTRADOS  (Arriba/Abajo para elegir tu piloto)");

    float x0 = 250, y0 = 335, ancho = 900, altoFila = 32;
    al_draw_filled_rectangle(x0, y0, x0 + ancho, y0 + altoFila, al_map_rgb(35, 40, 70));
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 20, y0 + 7, 0, "#");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 70, y0 + 7, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 260, y0 + 7, 0, "Vel.");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 360, y0 + 7, 0, "Destreza");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 510, y0 + 7, 0, "Tipo");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 650, y0 + 7, 0, "IA");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 790, y0 + 7, 0, "Extra");

    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        float y = y0 + altoFila * (i + 1);
        int esSeleccionado = (i == jugadorSeleccionado);
        ALLEGRO_COLOR fondo = esSeleccionado ? al_map_rgb(0, 60, 100)
            : (i % 2 == 0) ? al_map_rgb(22, 26, 50)
            : al_map_rgb(28, 32, 58);
        al_draw_filled_rectangle(x0, y, x0 + ancho, y + altoFila, fondo);
        if (esSeleccionado) al_draw_rectangle(x0, y, x0 + ancho, y + altoFila, al_map_rgb(0, 220, 255), 2.0f);

        ALLEGRO_COLOR texto = esSeleccionado ? al_map_rgb(0, 220, 255) : al_map_rgb(230, 230, 230);

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(normal, texto, x0 + 20, y + 7, 0, buf);
        snprintf(buf, sizeof(buf), "%s%s", a->nombre, esSeleccionado ? " (TU)" : "");
        al_draw_text(normal, texto, x0 + 70, y + 7, 0, buf);
        snprintf(buf, sizeof(buf), "%.0f", a->velocidadBase);
        al_draw_text(normal, texto, x0 + 260, y + 7, 0, buf);
        snprintf(buf, sizeof(buf), "%.0f", a->destreza);
        al_draw_text(normal, texto, x0 + 360, y + 7, 0, buf);
        al_draw_text(normal, texto, x0 + 510, y + 7, 0, a->tipoVehiculo == TIPO_DEPORTIVO ? "Deportivo" : "Todoterreno");
        al_draw_text(normal, texto, x0 + 650, y + 7, 0, personalidadNombre(a->personalidad));
        float extra = a->tipoVehiculo == TIPO_DEPORTIVO ? a->especificaciones.turbo : a->especificaciones.traccion;
        snprintf(buf, sizeof(buf), "%.0f", extra);
        al_draw_text(normal, texto, x0 + 790, y + 7, 0, buf);
    }

    float yControles = 700;
    al_draw_filled_rounded_rectangle(250, yControles, 1150, yControles + 115, 15, 15, al_map_rgb(18, 22, 45));
    al_draw_rounded_rectangle(250, yControles, 1150, yControles + 115, 15, 15, al_map_rgb(255, 200, 0), 2);
    al_draw_text(normal, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, yControles + 15, ALLEGRO_ALIGN_CENTRE, "CONTROLES");
    al_draw_text(normal, al_map_rgb(230, 230, 230), ANCHO_VENTANA / 2, yControles + 45, ALLEGRO_ALIGN_CENTRE,
        "Carrera: Flecha arriba = nitro | Flecha abajo = frenar | P = pit stop en ventana");
    al_draw_text(normal, al_map_rgb(0, 220, 255), ANCHO_VENTANA / 2, yControles + 75, ALLEGRO_ALIGN_CENTRE,
        "Extras: pits en D/5, taller animado, DRS, safety car, clima dinamico y radio");
}

static void dibujarFin(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal,
    Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    al_clear_to_color(al_map_rgb(8, 10, 25));

    char buf[160], tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    al_draw_text(grande, al_map_rgb(255, 210, 40), ANCHO_VENTANA / 2, 70, ALLEGRO_ALIGN_CENTRE, "BANDERA A CUADROS");

    if (ganadorIdx >= 0 && ganadorIdx < cfg->numAutos) {
        snprintf(buf, sizeof(buf), "Ganador: %s", cfg->autos[ganadorIdx].nombre);
        al_draw_text(grande, al_map_rgb(0, 220, 255), ANCHO_VENTANA / 2, 150, ALLEGRO_ALIGN_CENTRE, buf);
        snprintf(buf, sizeof(buf), "%d turnos | Tiempo oficial: %s", turno, tiempoTxt);
        al_draw_text(normal, al_map_rgb(230, 230, 230), ANCHO_VENTANA / 2, 210, ALLEGRO_ALIGN_CENTRE, buf);
    }

    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);
    al_draw_text(normal, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 270, ALLEGRO_ALIGN_CENTRE, "PODIO");

    if (cnt >= 1) { snprintf(buf, sizeof(buf), "1. %s", orden[0].nombre); al_draw_text(grande, al_map_rgb(255, 215, 0), ANCHO_VENTANA / 2, 320, ALLEGRO_ALIGN_CENTRE, buf); }
    if (cnt >= 2) { snprintf(buf, sizeof(buf), "2. %s", orden[1].nombre); al_draw_text(normal, al_map_rgb(210, 210, 210), ANCHO_VENTANA / 2, 390, ALLEGRO_ALIGN_CENTRE, buf); }
    if (cnt >= 3) { snprintf(buf, sizeof(buf), "3. %s", orden[2].nombre); al_draw_text(normal, al_map_rgb(205, 127, 50), ANCHO_VENTANA / 2, 430, ALLEGRO_ALIGN_CENTRE, buf); }

    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE, "ENTER = volver al menu | ESC = salir");
    al_draw_text(normal, al_map_rgb(150, 220, 150), ANCHO_VENTANA / 2, ALTO_VENTANA - 35, ALLEGRO_ALIGN_CENTRE, "Se guardaron reporte, mejores marcas, temporada y estadisticas");
}

/* REPORTE */
void generarReporteFinal(Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    FILE* f = fopen("reporte.txt", "w");
    if (!f) { printf("No se pudo generar reporte.txt\n"); return; }

    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    fprintf(f, "=============================================\n");
    fprintf(f, "          REPORTE FINAL DE CARRERA F1\n");
    fprintf(f, "=============================================\n\n");
    fprintf(f, "Clima final: %s\n", climaNombre(cfg->clima));
    fprintf(f, "Longitud de pista: %d unidades\n", cfg->longitudPista);
    fprintf(f, "Turnos totales: %d\n", turno);
    fprintf(f, "Tiempo oficial: %s\n\n", tiempoTxt);
    if (ganadorIdx >= 0) fprintf(f, "Ganador: %s\n\n", cfg->autos[ganadorIdx].nombre);

    Auto orden[MAX_AUTOS];
    int cnt = ordenarPorPosicion(cfg, orden);
    fprintf(f, "CLASIFICACION FINAL:\n");
    fprintf(f, "---------------------------------------------\n");
    for (int i = 0; i < cnt; i++)
        fprintf(f, "%d. %s | %.1f unidades | VelMax: %.1f | Rebases: %d | Accidentes: %d | Pits: %d | Neum: %.1f | Fuel: %.1f\n",
            i + 1, orden[i].nombre, orden[i].posicion, orden[i].velocidadMaxima,
            orden[i].adelantamientos, orden[i].accidentesTotales, orden[i].pitsRealizados,
            orden[i].neumaticos, orden[i].combustible);

    fprintf(f, "\n=============================================\n");
    fprintf(f, "Reporte generado automaticamente por el simulador.\n");
    fprintf(f, "=============================================\n");
    fclose(f);
}

/* MAIN */
int main(void)
{
    srand((unsigned int)time(NULL));

    if (!al_init()) { fprintf(stderr, "al_init fallo\n"); return 1; }

    al_install_keyboard();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    inicializarColores();

    ALLEGRO_DISPLAY* display = al_create_display(ANCHO_VENTANA, ALTO_VENTANA);
    if (!display) { fprintf(stderr, "No se pudo crear ventana\n"); return 1; }
    al_set_window_title(display, "Simulador F1");

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* cola = al_create_event_queue();
    al_register_event_source(cola, al_get_keyboard_event_source());
    al_register_event_source(cola, al_get_timer_event_source(timer));
    al_register_event_source(cola, al_get_display_event_source(display));
    al_start_timer(timer);

    ALLEGRO_FONT* fNormal = al_load_ttf_font("DejaVuSans.ttf", 16, 0);
    ALLEGRO_FONT* fGrande = al_load_ttf_font("DejaVuSans-Bold.ttf", 32, 0);
    if (!fNormal) fNormal = al_create_builtin_font();
    if (!fGrande) fGrande = al_create_builtin_font();

    Configuracion cfg;
    if (!leerConfiguracion("config.txt", &cfg)) { fprintf(stderr, "No se pudo leer config.txt\n"); return 1; }

    EstadoJuego estado = ESTADO_MENU;
    int turno = 0, ganadorIdx = -1, salir = 0, redibujar = 0;
    int teclaArriba = 0, teclaAbajo = 0, teclaPit = 0;
    int framesPorTurno = 8, frameContador = 0;
    float tiempoCarrera = 0.0f;

    while (!salir) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(cola, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) salir = 1;

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (ev.keyboard.keycode) {
            case ALLEGRO_KEY_ESCAPE: salir = 1; break;

            case ALLEGRO_KEY_ENTER:
                if (estado == ESTADO_MENU) {
                    /* Aplicar seleccion de jugador */
                    for (int i = 0; i < cfg.numAutos; i++)
                        cfg.autos[i].esJugador = (i == jugadorSeleccionado) ? 1 : 0;

                    for (int i = 0; i < cfg.numAutos; i++) {
                        cfg.autos[i].posicion = 0;
                        cfg.autos[i].velocidadActual = 0;
                        cfg.autos[i].accidentado = 0;
                        cfg.autos[i].turnosAccidente = 0;
                        cfg.autos[i].nitro = cfg.autos[i].esJugador ? NITRO_MAX : 0;
                        cfg.autos[i].nitroUsado = 0;
                        cfg.autos[i].adelantamientos = 0;
                        cfg.autos[i].accidentesTotales = 0;
                        cfg.autos[i].velocidadMaxima = 0;
                        cfg.autos[i].neumaticos = NEUMATICOS_MAX;
                        cfg.autos[i].enPits = 0;
                        cfg.autos[i].turnosPit = 0;
                        cfg.autos[i].pitsRealizados = 0;
                        cfg.autos[i].animacionPit = 0;
                        cfg.autos[i].entrandoPits = 0;
                        cfg.autos[i].saliendoPits = 0;
                        for (int p = 0; p < PIT_MAX_PARADAS; p++) cfg.autos[i].penalizacionPitAplicada[p] = 0;
                        cfg.autos[i].combustible = FUEL_MAX;
                        cfg.autos[i].eliminado = 0;
                        cfg.autos[i].drsActivo = 0;
                    }
                    turno = 0; ganadorIdx = -1; tiempoCarrera = 0.0f; safetyCarTurnos = 0;
                    strcpy(ultimoEvento, "Carrera iniciada.");
                    strcpy(radioEquipo, "Ingeniero: Buena salida, administra el ritmo.");
                    system("cls");
                    estado = ESTADO_CORRIENDO;
                }
                else if (estado == ESTADO_FIN || estado == ESTADO_MARCAS ||
                    estado == ESTADO_TEMPORADA || estado == ESTADO_ESTADISTICAS) {
                    estado = ESTADO_MENU;
                }
                break;

            case ALLEGRO_KEY_UP:
                if (estado == ESTADO_MENU) {
                    jugadorSeleccionado--;
                    if (jugadorSeleccionado < 0) jugadorSeleccionado = cfg.numAutos - 1;
                }
                else { teclaArriba = 1; }
                break;

            case ALLEGRO_KEY_DOWN:
                if (estado == ESTADO_MENU) {
                    jugadorSeleccionado++;
                    if (jugadorSeleccionado >= cfg.numAutos) jugadorSeleccionado = 0;
                }
                else { teclaAbajo = 1; }
                break;

            case ALLEGRO_KEY_P: teclaPit = 1; break;

            case ALLEGRO_KEY_A:
                if (estado == ESTADO_MENU)
                {
                    agregarAutoManual(&cfg);
                    redibujar = 1;
                }
                break;

            case ALLEGRO_KEY_M: if (estado == ESTADO_MENU) estado = ESTADO_MARCAS;       break;
            case ALLEGRO_KEY_T: if (estado == ESTADO_MENU) estado = ESTADO_TEMPORADA;    break;
            case ALLEGRO_KEY_H: if (estado == ESTADO_MENU) estado = ESTADO_ESTADISTICAS; break;
            }
        }

        if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            if (estado != ESTADO_MENU) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_UP)   teclaArriba = 0;
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) teclaAbajo = 0;
            }
            if (ev.keyboard.keycode == ALLEGRO_KEY_P) teclaPit = 0;
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redibujar = 1;
            if (estado == ESTADO_CORRIENDO) {
                if (++frameContador >= framesPorTurno) {
                    frameContador = 0;
                    turno++;
                    tiempoCarrera += framesPorTurno / (float)FPS;

                    recargarNitroJugador(&cfg, turno);
                    actualizarClimaDinamico(&cfg, turno);
                    actualizarRadio(&cfg, turno);
                    revisarPitsObligatorios(&cfg);
                    actualizarPosiciones(&cfg, teclaArriba, teclaAbajo, teclaPit, turno);
                    imprimirConsolaEstado(&cfg, turno, tiempoCarrera);

                    ganadorIdx = determinarGanador(&cfg);
                    if (ganadorIdx >= 0) {
                        imprimirConsolaFinal(&cfg, ganadorIdx, turno, tiempoCarrera);
                        guardarMarca(cfg.autos[ganadorIdx], turno, tiempoCarrera);
                        guardarPuntosTemporada(&cfg);
                        guardarEstadisticasHistoricas(&cfg, ganadorIdx, tiempoCarrera);
                        generarReporteFinal(&cfg, ganadorIdx, turno, tiempoCarrera);
                        estado = ESTADO_FIN;
                    }
                }
            }
        }

        if (redibujar && al_is_event_queue_empty(cola)) {
            redibujar = 0;
            al_clear_to_color(al_map_rgb(15, 15, 25));
            switch (estado) {
            case ESTADO_MENU:          dibujarMenu(fGrande, fNormal, &cfg);                                    break;
            case ESTADO_CORRIENDO:     dibujarPista(&cfg, fNormal, turno, tiempoCarrera); dibujarTabla(&cfg, fNormal); break;
            case ESTADO_FIN:           dibujarFin(fGrande, fNormal, &cfg, ganadorIdx, turno, tiempoCarrera);   break;
            case ESTADO_MARCAS:        mostrarMejoresMarcas(fGrande, fNormal);                                 break;
            case ESTADO_TEMPORADA:     mostrarTemporada(fGrande, fNormal);                                     break;
            case ESTADO_ESTADISTICAS:  mostrarEstadisticas(fGrande, fNormal);                                  break;
            }
            al_flip_display();
        }
    }

    liberarConfiguracion(&cfg);
    al_destroy_font(fNormal);
    al_destroy_font(fGrande);
    al_destroy_timer(timer);
    al_destroy_event_queue(cola);
    al_destroy_display(display);
    return 0;
}