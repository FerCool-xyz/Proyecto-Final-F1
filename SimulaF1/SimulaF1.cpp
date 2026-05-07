// SIMULADOR DE CARRERA F1
// Programacion 1 - ISC 2C - Fernando Vicente Munoz - Alejandro Martinez Esparza

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
#define MAX_NOMBRE       32
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

#define NITRO_MAX          35
#define NITRO_RECARGA      2
#define NITRO_CADA_TURNOS  3

typedef enum
{
    CLIMA_SECO,
    CLIMA_LLUVIA,
    CLIMA_NIEVE
} Clima;

typedef enum
{
    ESTADO_MENU,
    ESTADO_CORRIENDO,
    ESTADO_FIN,
    ESTADO_MARCAS
} EstadoJuego;

/* Union de especificaciones */
typedef union
{
    float turbo;
    float traccion;
} Especificaciones;

/* Estructura Auto */
typedef struct
{
    char nombre[MAX_NOMBRE];
    float velocidadBase;
    float destreza;
    char tipoVehiculo;
    Especificaciones especificaciones;

    float posicion;
    float velocidadActual;
    int accidentado;
    int turnosAccidente;
    int esJugador;

    int nitro;
    int nitroUsado;
    int adelantamientos;
    int accidentesTotales;
    float velocidadMaxima;

    ALLEGRO_COLOR color;
} Auto;

/* Configuracion de carrera */
typedef struct
{
    int longitudPista;
    Clima clima;
    int numAutos;
    Auto* autos;
} Configuracion;

/* Marca guardada en archivo binario */
typedef struct
{
    char nombre[MAX_NOMBRE];
    int turnos;
    float tiempo;
    float velocidadMaxima;
    int accidentes;
    int adelantamientos;
} Marca;

/* PALETA DE COLORES */
static ALLEGRO_COLOR COLORES_AUTO[8];

static void inicializarColores(void)
{
    COLORES_AUTO[0] = al_map_rgb(220, 50, 50);
    COLORES_AUTO[1] = al_map_rgb(50, 140, 255);
    COLORES_AUTO[2] = al_map_rgb(50, 220, 50);
    COLORES_AUTO[3] = al_map_rgb(255, 220, 20);
    COLORES_AUTO[4] = al_map_rgb(255, 130, 0);
    COLORES_AUTO[5] = al_map_rgb(200, 50, 230);
    COLORES_AUTO[6] = al_map_rgb(0, 220, 220);
    COLORES_AUTO[7] = al_map_rgb(255, 100, 180);
}

static const char* climaNombre(Clima c)
{
    switch (c)
    {
    case CLIMA_LLUVIA:
        return "LLUVIA";

    case CLIMA_NIEVE:
        return "NIEVE";

    default:
        return "SECO";
    }
}

void formatearTiempo(float tiempo, char* buffer, int tam)
{
    int minutos = (int)(tiempo / 60.0f);
    float segundos = tiempo - (minutos * 60.0f);

    snprintf(buffer, tam, "%02d:%06.3f", minutos, segundos);
}

void guardarMarca(Auto ganador, int turnos, float tiempo)
{
    FILE* f = fopen("marcas.dat", "ab");

    if (!f)
    {
        printf("No se pudo abrir marcas.dat\n");
        return;
    }

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

    al_draw_text(grande, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 50, ALLEGRO_ALIGN_CENTRE,
        "MEJORES MARCAS");

    FILE* f = fopen("marcas.dat", "rb");

    if (!f)
    {
        al_draw_text(normal, al_map_rgb(220, 220, 220),
            ANCHO_VENTANA / 2, 160, ALLEGRO_ALIGN_CENTRE,
            "Todavia no hay marcas registradas.");

        al_draw_text(normal, al_map_rgb(180, 180, 180),
            ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE,
            "Presiona ENTER para volver al menu");

        return;
    }

    Marca marcas[100];
    int total = 0;

    while (total < 100 && fread(&marcas[total], sizeof(Marca), 1, f) == 1)
    {
        total++;
    }

    fclose(f);

    for (int i = 0; i < total - 1; i++)
    {
        for (int j = 0; j < total - 1 - i; j++)
        {
            if (marcas[j].tiempo > marcas[j + 1].tiempo)
            {
                Marca temp = marcas[j];
                marcas[j] = marcas[j + 1];
                marcas[j + 1] = temp;
            }
        }
    }

    al_draw_text(normal, al_map_rgb(255, 200, 0), 120, 120, 0, "#");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 190, 120, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 410, 120, 0, "Turnos");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 540, 120, 0, "Tiempo");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 700, 120, 0, "Vel. Max");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 860, 120, 0, "Accidentes");
    al_draw_text(normal, al_map_rgb(255, 200, 0), 1030, 120, 0, "Rebases");

    int limite = total < 10 ? total : 10;

    for (int i = 0; i < limite; i++)
    {
        char buf[128];
        char tiempoTxt[32];
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

    al_draw_text(normal, al_map_rgb(180, 180, 180),
        ANCHO_VENTANA / 2, ALTO_VENTANA - 60, ALLEGRO_ALIGN_CENTRE,
        "Presiona ENTER para volver al menu");
}

/* Lee config.txt */
int leerConfiguracion(const char* archivo, Configuracion* cfg)
{
    FILE* f = fopen(archivo, "r");

    if (!f)
    {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", archivo);
        return 0;
    }

    cfg->longitudPista = 500;
    cfg->clima = CLIMA_SECO;
    cfg->numAutos = 0;
    cfg->autos = NULL;

    char linea[256];
    int conteo = 0;

    while (fgets(linea, sizeof(linea), f))
    {
        if (strncmp(linea, "auto=", 5) == 0)
        {
            conteo++;
        }
    }

    rewind(f);

    cfg->autos = (Auto*)malloc(sizeof(Auto) * (conteo > 0 ? conteo : 1));

    if (!cfg->autos)
    {
        fclose(f);
        return 0;
    }

    int idx = 0;

    while (fgets(linea, sizeof(linea), f))
    {
        if (linea[0] == '#' || linea[0] == '\n')
        {
            continue;
        }

        if (strncmp(linea, "longitud_pista=", 15) == 0)
        {
            cfg->longitudPista = atoi(linea + 15);
        }
        else if (strncmp(linea, "clima=", 6) == 0)
        {
            char cs[16];
            sscanf(linea + 6, "%15s", cs);

            if (strcmp(cs, "LLUVIA") == 0)
            {
                cfg->clima = CLIMA_LLUVIA;
            }
            else if (strcmp(cs, "NIEVE") == 0)
            {
                cfg->clima = CLIMA_NIEVE;
            }
            else
            {
                cfg->clima = CLIMA_SECO;
            }
        }
        else if (strncmp(linea, "auto=", 5) == 0 && idx < conteo)
        {
            Auto* a = &cfg->autos[idx];
            memset(a, 0, sizeof(Auto));

            char tipo;
            float extra;

            sscanf(linea + 5, "%31[^,],%f,%f,%c,%f",
                a->nombre,
                &a->velocidadBase,
                &a->destreza,
                &tipo,
                &extra);

            a->tipoVehiculo = tipo;

            if (tipo == TIPO_DEPORTIVO)
            {
                a->especificaciones.turbo = extra;
            }
            else
            {
                a->especificaciones.traccion = extra;
            }

            a->color = COLORES_AUTO[idx % 8];

            idx++;
        }
    }

    fclose(f);

    cfg->numAutos = idx;

    if (cfg->numAutos > 0)
    {
        cfg->autos[0].esJugador = 1;
    }

    return 1;
}

static float factorClima(Clima clima, char tipo)
{
    if (clima == CLIMA_LLUVIA)
    {
        return (tipo == TIPO_TODOTERRENO) ? 0.95f : 0.80f;
    }

    if (clima == CLIMA_NIEVE)
    {
        return (tipo == TIPO_TODOTERRENO) ? 0.85f : 0.60f;
    }

    return 1.0f;
}


void recargarNitroJugador(Configuracion* cfg, int turno)
{
    if (turno % NITRO_CADA_TURNOS != 0)
    {
        return;
    }

    for (int i = 0; i < cfg->numAutos; i++)
    {
        if (cfg->autos[i].esJugador)
        {
            if (cfg->autos[i].nitro < NITRO_MAX)
            {
                cfg->autos[i].nitro += NITRO_RECARGA;

                if (cfg->autos[i].nitro > NITRO_MAX)
                {
                    cfg->autos[i].nitro = NITRO_MAX;
                }
            }

            break;
        }
    }
}

void actualizarPosiciones(Configuracion* cfg, int teclaArriba, int teclaAbajo)
{
    for (int i = 0; i < cfg->numAutos; i++)
    {
        Auto* a = &cfg->autos[i];
        float posicionAnterior = a->posicion;

        if (a->accidentado)
        {
            if (--a->turnosAccidente <= 0)
            {
                a->accidentado = 0;
            }

            a->velocidadActual = 0;
            continue;
        }

        float vel = a->velocidadBase;

        if (a->tipoVehiculo == TIPO_DEPORTIVO)
        {
            vel += a->especificaciones.turbo * 0.05f;
        }
        else
        {
            vel += a->especificaciones.traccion * 0.03f;
        }

        int usandoNitro = 0;

        if (a->esJugador)
        {
            if (teclaArriba && a->nitro > 0)
            {
                usandoNitro = 1;
                a->nitro--;
                a->nitroUsado++;
            }

            if (teclaAbajo)
            {
                vel *= 0.70f;
            }

            float lider = 0.0f;

            for (int j = 0; j < cfg->numAutos; j++)
            {
                if (cfg->autos[j].posicion > lider)
                {
                    lider = cfg->autos[j].posicion;
                }
            }

            float diferenciaLider = lider - a->posicion;

            if (diferenciaLider > 800)
            {
                vel *= 1.18f;
            }
            else if (diferenciaLider > 400)
            {
                vel *= 1.10f;
            }
        }
        else
        {
            float lider = 0.0f;
            float masCercanoAdelante = 999999.0f;
            int vaUltimo = 1;

            for (int j = 0; j < cfg->numAutos; j++)
            {
                if (cfg->autos[j].posicion > lider)
                {
                    lider = cfg->autos[j].posicion;
                }

                if (cfg->autos[j].posicion < a->posicion)
                {
                    vaUltimo = 0;
                }

                float dif = cfg->autos[j].posicion - a->posicion;

                if (dif > 0 && dif < masCercanoAdelante)
                {
                    masCercanoAdelante = dif;
                }
            }

            float diferenciaLider = lider - a->posicion;

            if (diferenciaLider > 400)
            {
                vel *= 1.30f;
            }
            else if (diferenciaLider > 200)
            {
                vel *= 1.18f;
            }
            else if (diferenciaLider > 100)
            {
                vel *= 1.10f;
            }

            if (vaUltimo)
            {
                vel *= 1.15f;
            }

            if (masCercanoAdelante < 90)
            {
                vel *= 1.12f;
            }

            if (masCercanoAdelante < 35)
            {
                vel *= 1.18f;
            }

            if (cfg->clima == CLIMA_LLUVIA && a->destreza < 90)
            {
                vel *= 0.92f;
            }

            if (cfg->clima == CLIMA_NIEVE && a->tipoVehiculo == TIPO_DEPORTIVO)
            {
                vel *= 0.88f;
            }

            float estrategia = ((float)rand() / RAND_MAX) * 0.20f - 0.05f;
            vel *= (1.0f + estrategia);
        }

        vel *= factorClima(cfg->clima, a->tipoVehiculo);

        if (a->esJugador && usandoNitro)
        {
            vel *= 1.60f;
            vel += 20.0f;
        }

        if (rand() % 100 < 8)
        {
            int evento = rand() % 3;

            if (evento == 0)
            {
                vel *= 1.25f;
            }
            else if (evento == 1)
            {
                vel *= 0.80f;
            }
            else
            {
                vel *= 1.12f;
            }
        }

        if (vel < 5.0f)
        {
            vel = 5.0f;
        }

        a->velocidadActual = vel;
        a->posicion += vel;

        if (a->velocidadActual > a->velocidadMaxima)
        {
            a->velocidadMaxima = a->velocidadActual;
        }

        float prob = (100.0f - a->destreza) / 1200.0f;

        if (a->esJugador && usandoNitro)
        {
            prob *= 1.8f;
        }

        if (!a->esJugador && vel > a->velocidadBase * 1.25f)
        {
            prob *= 1.4f;
        }

        if (cfg->clima == CLIMA_LLUVIA)
        {
            prob *= 1.6f;
        }

        if (cfg->clima == CLIMA_NIEVE)
        {
            prob *= 2.7f;
        }

        if ((float)rand() / RAND_MAX < prob)
        {
            a->accidentado = 1;
            a->turnosAccidente = 2 + rand() % 3;
            a->accidentesTotales++;
        }

        for (int j = 0; j < cfg->numAutos; j++)
        {
            if (i == j)
            {
                continue;
            }

            if (posicionAnterior < cfg->autos[j].posicion &&
                a->posicion > cfg->autos[j].posicion)
            {
                a->adelantamientos++;
            }
        }
    }
}

int determinarGanador(Configuracion* cfg)
{
    for (int i = 0; i < cfg->numAutos; i++)
    {
        if (cfg->autos[i].posicion >= cfg->longitudPista)
        {
            return i;
        }
    }

    return -1;
}

void liberarConfiguracion(Configuracion* cfg)
{
    if (cfg->autos)
    {
        free(cfg->autos);
        cfg->autos = NULL;
    }

    cfg->numAutos = 0;
}

void imprimirConsolaFinal(Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    system("cls");

    Auto orden[MAX_AUTOS];
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;

    for (int i = 0; i < cnt; i++)
    {
        orden[i] = cfg->autos[i];
    }

    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - 1 - i; j++)
        {
            if (orden[j].posicion < orden[j + 1].posicion)
            {
                Auto temp = orden[j];
                orden[j] = orden[j + 1];
                orden[j + 1] = temp;
            }
        }
    }

    printf("==========================================================================\n");
    printf("                            BANDERA A CUADROS                             \n");
    printf("==========================================================================\n\n");

    if (ganadorIdx >= 0)
    {
        printf(" GANADOR: %s\n", cfg->autos[ganadorIdx].nombre);
    }

    printf(" TURNOS : %d\n", turno);
    printf(" TIEMPO : %s\n\n", tiempoTxt);

    printf(" PODIO\n");
    printf("--------------------------------------------------------------------------\n");

    if (cnt >= 1)
    {
        printf(" 1. %s\n", orden[0].nombre);
    }

    if (cnt >= 2)
    {
        printf(" 2. %s\n", orden[1].nombre);
    }

    if (cnt >= 3)
    {
        printf(" 3. %s\n", orden[2].nombre);
    }

    printf("\n CLASIFICACION FINAL\n");
    printf("--------------------------------------------------------------------------\n");
    printf(" %-3s | %-14s | %-10s | %-8s | %-8s | %-8s\n",
        "POS", "Piloto", "Distancia", "VelMax", "Rebases", "Choques");

    for (int i = 0; i < cnt; i++)
    {
        printf(" %-3d | %-14s | %10.1f | %8.1f | %8d | %8d\n",
            i + 1,
            orden[i].nombre,
            orden[i].posicion,
            orden[i].velocidadMaxima,
            orden[i].adelantamientos,
            orden[i].accidentesTotales);
    }

    printf("==========================================================================\n");
}

void imprimirConsolaEstado(Configuracion* cfg, int turno, float tiempoCarrera)
{
    system("cls");

    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    Auto orden[MAX_AUTOS];
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;

    for (int i = 0; i < cnt; i++)
    {
        orden[i] = cfg->autos[i];
    }

    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - 1 - i; j++)
        {
            if (orden[j].posicion < orden[j + 1].posicion)
            {
                Auto tmp = orden[j];
                orden[j] = orden[j + 1];
                orden[j + 1] = tmp;
            }
        }
    }

    float posicionLider = orden[0].posicion;

    printf("==========================================================================\n");
    printf("                         SIMULADOR DE CARRERA F1                         \n");
    printf("==========================================================================\n");
    printf(" Turno: %-5d | Tiempo: %-10s | Clima: %-8s | Pista: %d unidades\n",
        turno, tiempoTxt, climaNombre(cfg->clima), cfg->longitudPista);
    printf("--------------------------------------------------------------------------\n");
    printf(" %-3s | %-14s | %-13s | %-8s | %-10s | %-9s | %-8s\n",
        "POS", "Piloto", "Distancia", "Vel.", "Gap", "Ritmo", "Estado");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < cnt; i++)
    {
        Auto* a = &orden[i];

        char diferencia[32];

        if (i == 0)
        {
            strcpy(diferencia, "Lider");
        }
        else
        {
            snprintf(diferencia, sizeof(diferencia), "+%.1f", posicionLider - a->posicion);
        }

        const char* ritmo;

        if (a->velocidadActual >= 140)
        {
            ritmo = "Rapido";
        }
        else if (a->velocidadActual >= 100)
        {
            ritmo = "Medio";
        }
        else
        {
            ritmo = "Lento";
        }

        printf(" %-3d | %-14s | %7.1f/%-5d | %7.1f | %-10s | %-9s | %-8s\n",
            i + 1,
            a->nombre,
            a->posicion,
            cfg->longitudPista,
            a->velocidadActual,
            diferencia,
            ritmo,
            a->accidentado ? "CRASH" : "OK");
    }

    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < cfg->numAutos; i++)
    {
        if (cfg->autos[i].esJugador)
        {
            Auto* j = &cfg->autos[i];

            float progreso = (j->posicion / cfg->longitudPista) * 100.0f;

            if (progreso > 100.0f)
            {
                progreso = 100.0f;
            }

            int barrasProgreso = (int)(progreso / 5.0f);
            int barrasNitro = (j->nitro * 20) / NITRO_MAX;

            if (barrasNitro > 20)
            {
                barrasNitro = 20;
            }

            printf(" Tu auto: %s\n", j->nombre);

            printf(" Progreso: [");

            for (int b = 0; b < 20; b++)
            {
                printf(b < barrasProgreso ? "#" : "-");
            }

            printf("] %.1f%%\n", progreso);

            printf(" Nitro:    [");

            for (int b = 0; b < 20; b++)
            {
                printf(b < barrasNitro ? "#" : "-");
            }

            printf("] %d restantes\n", j->nitro);

            printf(" Estadisticas: Nitro usado: %d | Rebases: %d | Accidentes: %d | Vel. Max: %.1f\n",
                j->nitroUsado,
                j->adelantamientos,
                j->accidentesTotales,
                j->velocidadMaxima);

            break;
        }
    }

    printf("==========================================================================\n");
    printf(" Controles: Flecha arriba = NITRO | Flecha abajo = frenar | ESC = salir | Nitro se recarga\n");
    printf("==========================================================================\n");

    fflush(stdout);
}

static void dibujarAuto(float cx, float cy, ALLEGRO_COLOR color,
    int esJugador, int accidentado,
    const char* inicial, ALLEGRO_FONT* fuente)
{
    ALLEGRO_COLOR c = accidentado ? al_map_rgb(255, 80, 80) : color;
    float w = 36;
    float h = 18;

    al_draw_filled_rounded_rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2, 5, 5, c);
    al_draw_filled_rounded_rectangle(cx - 8, cy - h / 2 - 6, cx + 8, cy - h / 2 + 2, 3, 3,
        al_map_rgba(255, 255, 255, 120));

    ALLEGRO_COLOR rueda = al_map_rgb(30, 30, 30);

    al_draw_filled_circle(cx - w / 2 + 5, cy + h / 2 - 2, 5, rueda);
    al_draw_filled_circle(cx + w / 2 - 5, cy + h / 2 - 2, 5, rueda);

    if (esJugador)
    {
        al_draw_rounded_rectangle(cx - w / 2 - 2, cy - h / 2 - 8, cx + w / 2 + 2, cy + h / 2 + 2,
            6, 6, al_map_rgb(0, 220, 255), 2.0f);
    }

    if (fuente)
    {
        al_draw_text(fuente, al_map_rgb(10, 10, 10), cx, cy - 6,
            ALLEGRO_ALIGN_CENTRE, inicial);
    }

    if (accidentado && fuente)
    {
        al_draw_text(fuente, al_map_rgb(255, 80, 80), cx, cy - h / 2 - 20,
            ALLEGRO_ALIGN_CENTRE, "X");
    }
}

void dibujarPista(Configuracion* cfg, ALLEGRO_FONT* fuente, int turno, float tiempoCarrera)
{
    int n = cfg->numAutos;
    float escala = 2.0f;
    float camara = 0.0f;

    for (int j = 0; j < n; j++)
    {
        if (cfg->autos[j].esJugador)
        {
            camara = cfg->autos[j].posicion - 250.0f;

            if (camara < 0)
            {
                camara = 0;
            }

            break;
        }
    }

    al_draw_filled_rectangle(0, MARGEN_Y - 20,
        ANCHO_VENTANA, MARGEN_Y + n * ALTO_CARRIL + 20,
        al_map_rgb(30, 80, 30));

    for (int i = 0; i < n; i++)
    {
        float y0 = MARGEN_Y + i * ALTO_CARRIL;
        ALLEGRO_COLOR col = (i % 2 == 0) ? al_map_rgb(50, 50, 65) : al_map_rgb(42, 42, 55);

        al_draw_filled_rectangle(MARGEN_X, y0, MARGEN_X + ANCHO_PISTA, y0 + ALTO_CARRIL, col);

        for (int x = MARGEN_X - ((int)(camara * escala) % 40); x < MARGEN_X + ANCHO_PISTA; x += 40)
        {
            al_draw_filled_rectangle(x, y0 + ALTO_CARRIL / 2 - 1,
                x + 20, y0 + ALTO_CARRIL / 2 + 1,
                al_map_rgba(200, 200, 200, 60));
        }
    }

    float py0 = MARGEN_Y;
    float py1 = MARGEN_Y + n * ALTO_CARRIL;

    al_draw_rectangle(MARGEN_X, py0, MARGEN_X + ANCHO_PISTA, py1,
        al_map_rgb(200, 180, 50), 3.0f);

    float xMeta = MARGEN_X + (cfg->longitudPista - camara) * escala;

    if (xMeta >= MARGEN_X && xMeta <= MARGEN_X + ANCHO_PISTA)
    {
        for (int k = 0; k < (int)(py1 - py0); k += 10)
        {
            ALLEGRO_COLOR mc = (k / 10 % 2 == 0) ? al_map_rgb(255, 255, 255) : al_map_rgb(0, 0, 0);
            al_draw_filled_rectangle(xMeta - 5, py0 + k, xMeta + 5, py0 + k + 10, mc);
        }

        if (fuente)
        {
            al_draw_text(fuente, al_map_rgb(255, 255, 100), xMeta, py0 - 18,
                ALLEGRO_ALIGN_CENTRE, "META");
        }
    }

    if (fuente)
    {
        al_draw_text(fuente, al_map_rgb(100, 200, 100), MARGEN_X, py0 - 18,
            ALLEGRO_ALIGN_CENTRE, "SALIDA");
    }

    for (int i = 0; i < n; i++)
    {
        Auto* a = &cfg->autos[i];

        float px = MARGEN_X + (a->posicion - camara) * escala;
        float py = MARGEN_Y + i * ALTO_CARRIL + ALTO_CARRIL / 2.0f;

        if (px >= MARGEN_X - 40 && px <= MARGEN_X + ANCHO_PISTA + 40)
        {
            char ini[3] = { a->nombre[0], '\0' };
            dibujarAuto(px, py, a->color, a->esJugador, a->accidentado, ini, fuente);
        }
    }

    if (fuente)
    {
        char buf[128];
        char tiempoTxt[32];

        formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

        snprintf(buf, sizeof(buf), "Turno: %d | Tiempo: %s", turno, tiempoTxt);
        al_draw_text(fuente, al_map_rgb(230, 230, 230), 10, 10, 0, buf);

        snprintf(buf, sizeof(buf), "Clima: %s", climaNombre(cfg->clima));
        al_draw_text(fuente, al_map_rgb(255, 200, 0), 10, 28, 0, buf);

        for (int i = 0; i < cfg->numAutos; i++)
        {
            if (cfg->autos[i].esJugador)
            {
                char nitroTxt[64];
                snprintf(nitroTxt, sizeof(nitroTxt), "Nitro: %d", cfg->autos[i].nitro);
                al_draw_text(fuente, al_map_rgb(0, 220, 255), 10, 64, 0, nitroTxt);
                break;
            }
        }

        snprintf(buf, sizeof(buf), "Camara: %.0f / %d", camara, cfg->longitudPista);
        al_draw_text(fuente, al_map_rgb(200, 200, 200), 10, 46, 0, buf);

        al_draw_text(fuente, al_map_rgb(0, 220, 255),
            ANCHO_VENTANA - 10, 10, ALLEGRO_ALIGN_RIGHT,
            "Borde cian = TU auto");

        al_draw_text(fuente, al_map_rgb(200, 200, 200),
            ANCHO_VENTANA - 10, 28, ALLEGRO_ALIGN_RIGHT,
            "Arriba: NITRO  Abajo: frenar  ESC: salir");
    }
}

void dibujarTabla(Configuracion* cfg, ALLEGRO_FONT* fuente)
{
    if (!fuente)
    {
        return;
    }

    int n = cfg->numAutos;
    float ty = MARGEN_Y + n * ALTO_CARRIL + 30;
    float xs = MARGEN_X;
    float cw[6] = { 30, 160, 120, 110, 100, 90 };

    const char* hdr[] = { "#", "Piloto", "Posicion", "Vel.", "Rebases", "Estado" };
    float x = xs;

    for (int c = 0; c < 6; c++)
    {
        al_draw_text(fuente, al_map_rgb(255, 200, 0), x + 4, ty, 0, hdr[c]);
        x += cw[c];
    }

    ty += 18;
    al_draw_line(xs, ty, xs + 650, ty, al_map_rgb(120, 120, 60), 1.0f);
    ty += 4;

    Auto orden[MAX_AUTOS];
    int cnt = n < MAX_AUTOS ? n : MAX_AUTOS;

    for (int i = 0; i < cnt; i++)
    {
        orden[i] = cfg->autos[i];
    }

    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - 1 - i; j++)
        {
            if (orden[j].posicion < orden[j + 1].posicion)
            {
                Auto tmp = orden[j];
                orden[j] = orden[j + 1];
                orden[j + 1] = tmp;
            }
        }
    }

    for (int i = 0; i < cnt; i++)
    {
        Auto* a = &orden[i];
        char buf[32];
        x = xs;

        ALLEGRO_COLOR rc = a->esJugador ? al_map_rgb(0, 220, 255)
            : a->accidentado ? al_map_rgb(255, 80, 80)
            : al_map_rgb(230, 230, 230);

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf);
        x += cw[0];

        al_draw_text(fuente, rc, x + 4, ty, 0, a->nombre);
        x += cw[1];

        snprintf(buf, sizeof(buf), "%.0f/%d", a->posicion, cfg->longitudPista);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf);
        x += cw[2];

        snprintf(buf, sizeof(buf), "%.1f", a->velocidadActual);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf);
        x += cw[3];

        snprintf(buf, sizeof(buf), "%d", a->adelantamientos);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf);
        x += cw[4];

        al_draw_text(fuente, rc, x + 4, ty, 0, a->accidentado ? "CRASH" : "OK");

        ty += 20;
    }

    for (int i = 0; i < n; i++)
    {
        if (!cfg->autos[i].esJugador)
        {
            continue;
        }

        Auto* jug = &cfg->autos[i];
        ty += 8;

        float prog = jug->posicion / (float)cfg->longitudPista;

        if (prog > 1.0f)
        {
            prog = 1.0f;
        }

        float bw = 600;

        al_draw_text(fuente, al_map_rgb(0, 220, 255), xs, ty, 0, "Tu progreso:");
        ty += 16;

        al_draw_filled_rectangle(xs, ty, xs + bw, ty + 12, al_map_rgb(40, 40, 60));
        al_draw_filled_rectangle(xs, ty, xs + bw * prog, ty + 12, al_map_rgb(0, 220, 255));
        al_draw_rectangle(xs, ty, xs + bw, ty + 12, al_map_rgb(120, 120, 120), 1.0f);

        char pct[16];
        snprintf(pct, sizeof(pct), "%.0f%%", prog * 100.0f);
        al_draw_text(fuente, al_map_rgb(10, 10, 10),
            xs + bw * prog / 2.0f, ty, ALLEGRO_ALIGN_CENTRE, pct);

        break;
    }
}

static void dibujarMenu(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal, Configuracion* cfg)
{
    al_clear_to_color(al_map_rgb(8, 10, 25));

    for (int y = 0; y < ALTO_VENTANA; y++)
    {
        float t = (float)y / ALTO_VENTANA;
        al_draw_line(0, y, ANCHO_VENTANA, y,
            al_map_rgb((int)(8 + t * 20), (int)(10 + t * 15), (int)(25 + t * 35)), 1.0f);
    }

    al_draw_filled_rounded_rectangle(250, 40, 1150, 150, 20, 20, al_map_rgb(20, 25, 55));
    al_draw_rounded_rectangle(250, 40, 1150, 150, 20, 20, al_map_rgb(255, 200, 0), 3);

    al_draw_text(grande, al_map_rgb(255, 210, 40),
        ANCHO_VENTANA / 2, 65, ALLEGRO_ALIGN_CENTRE,
        "SIMULADOR DE CARRERA F1");

    al_draw_text(normal, al_map_rgb(210, 210, 210),
        ANCHO_VENTANA / 2, 115, ALLEGRO_ALIGN_CENTRE,
        "Proyecto Final - Programacion I");

    char buf[160];

    al_draw_filled_rounded_rectangle(330, 185, 1070, 265, 15, 15, al_map_rgb(18, 22, 45));
    al_draw_rounded_rectangle(330, 185, 1070, 265, 15, 15, al_map_rgb(80, 180, 255), 2);

    snprintf(buf, sizeof(buf), "Pista: %d unidades", cfg->longitudPista);
    al_draw_text(normal, al_map_rgb(230, 230, 230), 390, 205, 0, buf);

    snprintf(buf, sizeof(buf), "Clima: %s", climaNombre(cfg->clima));
    al_draw_text(normal, al_map_rgb(255, 200, 0), 650, 205, 0, buf);

    snprintf(buf, sizeof(buf), "Autos registrados: %d", cfg->numAutos);
    al_draw_text(normal, al_map_rgb(0, 220, 255), 870, 205, 0, buf);

    al_draw_text(normal, al_map_rgb(180, 180, 180),
        ANCHO_VENTANA / 2, 238, ALLEGRO_ALIGN_CENTRE,
        "El primer piloto registrado es el jugador");

    al_draw_text(normal, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 300, ALLEGRO_ALIGN_CENTRE,
        "PILOTOS REGISTRADOS");

    float x0 = 290;
    float y0 = 335;
    float ancho = 820;
    float altoFila = 32;

    al_draw_filled_rectangle(x0, y0, x0 + ancho, y0 + altoFila, al_map_rgb(35, 40, 70));

    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 20, y0 + 7, 0, "#");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 70, y0 + 7, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 270, y0 + 7, 0, "Vel.");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 390, y0 + 7, 0, "Destreza");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 550, y0 + 7, 0, "Tipo");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 670, y0 + 7, 0, "Extra");

    for (int i = 0; i < cfg->numAutos; i++)
    {
        Auto* a = &cfg->autos[i];
        float y = y0 + altoFila * (i + 1);

        ALLEGRO_COLOR fondo = (i % 2 == 0) ? al_map_rgb(22, 26, 50) : al_map_rgb(28, 32, 58);
        al_draw_filled_rectangle(x0, y, x0 + ancho, y + altoFila, fondo);

        ALLEGRO_COLOR texto = a->esJugador ? al_map_rgb(0, 220, 255) : al_map_rgb(230, 230, 230);

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(normal, texto, x0 + 20, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%s%s", a->nombre, a->esJugador ? "  (TU)" : "");
        al_draw_text(normal, texto, x0 + 70, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%.0f", a->velocidadBase);
        al_draw_text(normal, texto, x0 + 270, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%.0f", a->destreza);
        al_draw_text(normal, texto, x0 + 390, y + 7, 0, buf);

        al_draw_text(normal, texto, x0 + 550, y + 7, 0,
            a->tipoVehiculo == TIPO_DEPORTIVO ? "Deportivo" : "Todoterreno");

        float extra = a->tipoVehiculo == TIPO_DEPORTIVO ?
            a->especificaciones.turbo : a->especificaciones.traccion;

        snprintf(buf, sizeof(buf), "%.0f", extra);
        al_draw_text(normal, texto, x0 + 670, y + 7, 0, buf);
    }

    float yControles = 690;

    al_draw_filled_rounded_rectangle(310, yControles, 1090, yControles + 105, 15, 15,
        al_map_rgb(18, 22, 45));
    al_draw_rounded_rectangle(310, yControles, 1090, yControles + 105, 15, 15,
        al_map_rgb(255, 200, 0), 2);

    al_draw_text(normal, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, yControles + 15, ALLEGRO_ALIGN_CENTRE,
        "CONTROLES");

    al_draw_text(normal, al_map_rgb(230, 230, 230),
        ANCHO_VENTANA / 2, yControles + 45, ALLEGRO_ALIGN_CENTRE,
        "ENTER = iniciar carrera     M = mejores marcas     ESC = salir");

    al_draw_text(normal, al_map_rgb(0, 220, 255),
        ANCHO_VENTANA / 2, yControles + 72, ALLEGRO_ALIGN_CENTRE,
        "Durante la carrera: Flecha arriba = nitro     Flecha abajo = frenar     Nitro se recarga");
}

static void dibujarFin(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal,
    Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    al_clear_to_color(al_map_rgb(8, 10, 25));

    for (int y = 0; y < ALTO_VENTANA; y++)
    {
        float t = (float)y / ALTO_VENTANA;
        al_draw_line(0, y, ANCHO_VENTANA, y,
            al_map_rgb((int)(8 + t * 18), (int)(10 + t * 12), (int)(25 + t * 35)), 1.0f);
    }

    al_draw_filled_rounded_rectangle(260, 40, 1140, 150, 20, 20, al_map_rgb(20, 25, 55));
    al_draw_rounded_rectangle(260, 40, 1140, 150, 20, 20, al_map_rgb(255, 200, 0), 3);

    al_draw_text(grande, al_map_rgb(255, 210, 40),
        ANCHO_VENTANA / 2, 65, ALLEGRO_ALIGN_CENTRE,
        "BANDERA A CUADROS");

    char buf[160];
    char tiempoTxt[32];

    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    if (ganadorIdx >= 0 && ganadorIdx < cfg->numAutos)
    {
        Auto* g = &cfg->autos[ganadorIdx];

        snprintf(buf, sizeof(buf), "Ganador: %s", g->nombre);
        al_draw_text(grande, al_map_rgb(0, 220, 255),
            ANCHO_VENTANA / 2, 170, ALLEGRO_ALIGN_CENTRE, buf);

        snprintf(buf, sizeof(buf), "%d turnos | Tiempo oficial: %s", turno, tiempoTxt);
        al_draw_text(normal, al_map_rgb(230, 230, 230),
            ANCHO_VENTANA / 2, 225, ALLEGRO_ALIGN_CENTRE, buf);
    }

    Auto orden[MAX_AUTOS];
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;

    for (int i = 0; i < cnt; i++)
    {
        orden[i] = cfg->autos[i];
    }

    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - 1 - i; j++)
        {
            if (orden[j].posicion < orden[j + 1].posicion)
            {
                Auto temp = orden[j];
                orden[j] = orden[j + 1];
                orden[j + 1] = temp;
            }
        }
    }

    al_draw_text(normal, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 280, ALLEGRO_ALIGN_CENTRE,
        "PODIO");

    if (cnt >= 1)
    {
        al_draw_filled_rounded_rectangle(500, 340, 900, 400, 15, 15, al_map_rgb(35, 40, 70));
        snprintf(buf, sizeof(buf), "1. %s", orden[0].nombre);
        al_draw_text(grande, al_map_rgb(255, 215, 0),
            ANCHO_VENTANA / 2, 352, ALLEGRO_ALIGN_CENTRE, buf);
    }

    if (cnt >= 2)
    {
        al_draw_filled_rounded_rectangle(290, 430, 650, 485, 15, 15, al_map_rgb(30, 35, 60));
        snprintf(buf, sizeof(buf), "2. %s", orden[1].nombre);
        al_draw_text(normal, al_map_rgb(210, 210, 210),
            470, 448, ALLEGRO_ALIGN_CENTRE, buf);
    }

    if (cnt >= 3)
    {
        al_draw_filled_rounded_rectangle(750, 430, 1110, 485, 15, 15, al_map_rgb(30, 35, 60));
        snprintf(buf, sizeof(buf), "3. %s", orden[2].nombre);
        al_draw_text(normal, al_map_rgb(205, 127, 50),
            930, 448, ALLEGRO_ALIGN_CENTRE, buf);
    }

    float x0 = 260;
    float y0 = 535;
    float ancho = 880;
    float altoFila = 30;

    al_draw_text(normal, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 500, ALLEGRO_ALIGN_CENTRE,
        "CLASIFICACION FINAL");

    al_draw_filled_rectangle(x0, y0, x0 + ancho, y0 + altoFila, al_map_rgb(35, 40, 70));

    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 20, y0 + 7, 0, "POS");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 90, y0 + 7, 0, "Piloto");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 320, y0 + 7, 0, "Distancia");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 500, y0 + 7, 0, "Vel. Max");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 650, y0 + 7, 0, "Rebases");
    al_draw_text(normal, al_map_rgb(255, 200, 0), x0 + 760, y0 + 7, 0, "Choques");

    for (int i = 0; i < cnt; i++)
    {
        float y = y0 + altoFila * (i + 1);

        ALLEGRO_COLOR fondo = (i % 2 == 0) ? al_map_rgb(22, 26, 50) : al_map_rgb(28, 32, 58);
        ALLEGRO_COLOR texto = orden[i].esJugador ? al_map_rgb(0, 220, 255) : al_map_rgb(230, 230, 230);

        al_draw_filled_rectangle(x0, y, x0 + ancho, y + altoFila, fondo);

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(normal, texto, x0 + 20, y + 7, 0, buf);

        al_draw_text(normal, texto, x0 + 90, y + 7, 0, orden[i].nombre);

        snprintf(buf, sizeof(buf), "%.1f", orden[i].posicion);
        al_draw_text(normal, texto, x0 + 320, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%.1f", orden[i].velocidadMaxima);
        al_draw_text(normal, texto, x0 + 500, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%d", orden[i].adelantamientos);
        al_draw_text(normal, texto, x0 + 650, y + 7, 0, buf);

        snprintf(buf, sizeof(buf), "%d", orden[i].accidentesTotales);
        al_draw_text(normal, texto, x0 + 760, y + 7, 0, buf);
    }

    al_draw_text(normal, al_map_rgb(180, 180, 180),
        ANCHO_VENTANA / 2, ALTO_VENTANA - 50, ALLEGRO_ALIGN_CENTRE,
        "ENTER = volver al menu | ESC = salir");

    al_draw_text(normal, al_map_rgb(150, 220, 150),
        ANCHO_VENTANA / 2, ALTO_VENTANA - 25, ALLEGRO_ALIGN_CENTRE,
        "Se genero automaticamente el archivo reporte.txt");
}

void generarReporteFinal(Configuracion* cfg, int ganadorIdx, int turno, float tiempoCarrera)
{
    FILE* f = fopen("reporte.txt", "w");

    if (!f)
    {
        printf("No se pudo generar reporte.txt\n");
        return;
    }

    char tiempoTxt[32];
    formatearTiempo(tiempoCarrera, tiempoTxt, sizeof(tiempoTxt));

    fprintf(f, "=============================================\n");
    fprintf(f, "          REPORTE FINAL DE CARRERA F1\n");
    fprintf(f, "=============================================\n\n");

    fprintf(f, "Clima: %s\n", climaNombre(cfg->clima));
    fprintf(f, "Longitud de pista: %d unidades\n", cfg->longitudPista);
    fprintf(f, "Turnos totales: %d\n", turno);
    fprintf(f, "Tiempo oficial: %s\n\n", tiempoTxt);

    if (ganadorIdx >= 0)
    {
        fprintf(f, "Ganador: %s\n\n", cfg->autos[ganadorIdx].nombre);
    }

    Auto orden[MAX_AUTOS];
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;

    for (int i = 0; i < cnt; i++)
    {
        orden[i] = cfg->autos[i];
    }

    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - 1 - i; j++)
        {
            if (orden[j].posicion < orden[j + 1].posicion)
            {
                Auto temp = orden[j];
                orden[j] = orden[j + 1];
                orden[j + 1] = temp;
            }
        }
    }

    fprintf(f, "CLASIFICACION FINAL:\n");
    fprintf(f, "---------------------------------------------\n");

    for (int i = 0; i < cnt; i++)
    {
        fprintf(f, "%d. %s | %.1f unidades | Vel. Max: %.1f | Rebases: %d | Accidentes: %d\n",
            i + 1,
            orden[i].nombre,
            orden[i].posicion,
            orden[i].velocidadMaxima,
            orden[i].adelantamientos,
            orden[i].accidentesTotales);
    }

    fprintf(f, "\n=============================================\n");
    fprintf(f, "Reporte generado automaticamente por el simulador.\n");
    fprintf(f, "=============================================\n");

    fclose(f);
}

int main(void)
{
    srand((unsigned int)time(NULL));

    if (!al_init())
    {
        fprintf(stderr, "al_init fallo\n");
        return 1;
    }

    al_install_keyboard();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    inicializarColores();

    ALLEGRO_DISPLAY* display = al_create_display(ANCHO_VENTANA, ALTO_VENTANA);

    if (!display)
    {
        fprintf(stderr, "No se pudo crear ventana\n");
        return 1;
    }

    al_set_window_title(display, "Simulador F1");

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* cola = al_create_event_queue();

    al_register_event_source(cola, al_get_keyboard_event_source());
    al_register_event_source(cola, al_get_timer_event_source(timer));
    al_register_event_source(cola, al_get_display_event_source(display));
    al_start_timer(timer);

    ALLEGRO_FONT* fNormal = al_load_ttf_font("DejaVuSans.ttf", 16, 0);
    ALLEGRO_FONT* fGrande = al_load_ttf_font("DejaVuSans-Bold.ttf", 32, 0);

    if (!fNormal)
    {
        fNormal = al_create_builtin_font();
    }

    if (!fGrande)
    {
        fGrande = al_create_builtin_font();
    }

    Configuracion cfg;

    if (!leerConfiguracion("config.txt", &cfg))
    {
        fprintf(stderr, "No se pudo leer config.txt\n");
        return 1;
    }

    printf("Cargado: %d autos, pista=%d, clima=%s\n",
        cfg.numAutos, cfg.longitudPista, climaNombre(cfg.clima));

    EstadoJuego estado = ESTADO_MENU;
    int turno = 0;
    int ganadorIdx = -1;
    int salir = 0;
    int redibujar = 0;
    int teclaArriba = 0;
    int teclaAbajo = 0;
    int framesPorTurno = 8;
    int frameContador = 0;
    float tiempoCarrera = 0.0f;

    while (!salir)
    {
        ALLEGRO_EVENT ev;
        al_wait_for_event(cola, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            salir = 1;
        }

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            switch (ev.keyboard.keycode)
            {
            case ALLEGRO_KEY_ESCAPE:
                salir = 1;
                break;

            case ALLEGRO_KEY_ENTER:
                if (estado == ESTADO_MENU)
                {
                    for (int i = 0; i < cfg.numAutos; i++)
                    {
                        cfg.autos[i].posicion = 0;
                        cfg.autos[i].velocidadActual = 0;
                        cfg.autos[i].accidentado = 0;
                        cfg.autos[i].turnosAccidente = 0;
                        cfg.autos[i].nitro = cfg.autos[i].esJugador ? NITRO_MAX : 0;
                        cfg.autos[i].nitroUsado = 0;
                        cfg.autos[i].adelantamientos = 0;
                        cfg.autos[i].accidentesTotales = 0;
                        cfg.autos[i].velocidadMaxima = 0;
                    }

                    turno = 0;
                    ganadorIdx = -1;
                    tiempoCarrera = 0.0f;
                    estado = ESTADO_CORRIENDO;
                }
                else if (estado == ESTADO_FIN || estado == ESTADO_MARCAS)
                {
                    estado = ESTADO_MENU;
                }
                break;

            case ALLEGRO_KEY_UP:
                teclaArriba = 1;
                break;

            case ALLEGRO_KEY_DOWN:
                teclaAbajo = 1;
                break;

            case ALLEGRO_KEY_M:
                if (estado == ESTADO_MENU)
                {
                    estado = ESTADO_MARCAS;
                }
                break;
            }
        }

        if (ev.type == ALLEGRO_EVENT_KEY_UP)
        {
            if (ev.keyboard.keycode == ALLEGRO_KEY_UP)
            {
                teclaArriba = 0;
            }

            if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN)
            {
                teclaAbajo = 0;
            }
        }

        if (ev.type == ALLEGRO_EVENT_TIMER)
        {
            redibujar = 1;

            if (estado == ESTADO_CORRIENDO)
            {
                if (++frameContador >= framesPorTurno)
                {
                    frameContador = 0;
                    turno++;
                    tiempoCarrera += framesPorTurno / (float)FPS;

                    recargarNitroJugador(&cfg, turno);
                    actualizarPosiciones(&cfg, teclaArriba, teclaAbajo);
                    imprimirConsolaEstado(&cfg, turno, tiempoCarrera);

                    ganadorIdx = determinarGanador(&cfg);

                    if (ganadorIdx >= 0)
                    {
                        imprimirConsolaFinal(&cfg, ganadorIdx, turno, tiempoCarrera);
                        guardarMarca(cfg.autos[ganadorIdx], turno, tiempoCarrera);
                        generarReporteFinal(&cfg, ganadorIdx, turno, tiempoCarrera);

                        estado = ESTADO_FIN;
                    }
                }
            }
        }

        if (redibujar && al_is_event_queue_empty(cola))
        {
            redibujar = 0;
            al_clear_to_color(al_map_rgb(15, 15, 25));

            switch (estado)
            {
            case ESTADO_MENU:
                dibujarMenu(fGrande, fNormal, &cfg);
                break;

            case ESTADO_CORRIENDO:
                dibujarPista(&cfg, fNormal, turno, tiempoCarrera);
                dibujarTabla(&cfg, fNormal);
                break;

            case ESTADO_FIN:
                dibujarFin(fGrande, fNormal, &cfg, ganadorIdx, turno, tiempoCarrera);
                break;

            case ESTADO_MARCAS:
                mostrarMejoresMarcas(fGrande, fNormal);
                break;
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
