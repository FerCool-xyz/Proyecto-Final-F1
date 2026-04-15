/*
 * ============================================================
 *  SIMULADOR DE CARRERA F1  —  Archivo único
 *  Compilar con:
 *  gcc carrera_f1.c -o carrera_f1 -lallegro -lallegro_font
 *      -lallegro_ttf -lallegro_primitives -lallegro_color -mwindows
 * ============================================================
 */
#define _CRT_SECURE_NO_WARNINGS

 /* ─── Includes de Allegro 5 ─── */
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>

/* ─── Includes estándar ─── */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ════════════════════════════════════════════
   CONSTANTES
   ════════════════════════════════════════════ */
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

typedef enum { CLIMA_SECO, CLIMA_LLUVIA, CLIMA_NIEVE } Clima;
typedef enum { ESTADO_MENU, ESTADO_CORRIENDO, ESTADO_FIN } EstadoJuego;

//Unión de especificaciones (requisito del proyecto) 
typedef union {
    float turbo;    /* tipo D: boost de turbo  0-100 */
    float traccion; /* tipo T: agarre           0-100 */
} Especificaciones;

/* Estructura Auto (requisito del proyecto) */
typedef struct {
    char  nombre[MAX_NOMBRE];
    float velocidadBase;
    float destreza;
    char  tipoVehiculo;
    Especificaciones especificaciones;

    /* Estado dinámico */
    float posicion;
    float velocidadActual;
    int   accidentado;
    int   turnosAccidente;
    int   esJugador;
    int nitro;
    int nitroUsado;
    int adelantamientos;
    int accidentesTotales;
    float velocidadMaxima;
    /* Gráficos */
    ALLEGRO_COLOR color;
} Auto;

/* Configuración de carrera */
typedef struct {
    int   longitudPista;
    Clima clima;
    int   numAutos;
    Auto* autos;   /* arreglo dinámico (requisito del proyecto) */
} Configuracion;


  //PALETA DE COLORES

static ALLEGRO_COLOR COLORES_AUTO[8];

static void inicializarColores(void) {
    COLORES_AUTO[0] = al_map_rgb(220, 50, 50);
    COLORES_AUTO[1] = al_map_rgb(50, 140, 255);
    COLORES_AUTO[2] = al_map_rgb(50, 220, 50);
    COLORES_AUTO[3] = al_map_rgb(255, 220, 20);
    COLORES_AUTO[4] = al_map_rgb(255, 130, 0);
    COLORES_AUTO[5] = al_map_rgb(200, 50, 230);
    COLORES_AUTO[6] = al_map_rgb(0, 220, 220);
    COLORES_AUTO[7] = al_map_rgb(255, 100, 180);
}


static const char* climaNombre(Clima c) {
    switch (c) {
    case CLIMA_LLUVIA: return "LLUVIA";
    case CLIMA_NIEVE:  return "NIEVE";
    default:           return "SECO";
    }
}

//La config se encuentra en un archivo de texto(3er parcial)
int leerConfiguracion(const char* archivo, Configuracion* cfg) {
    FILE* f = fopen(archivo, "r");
    if (!f) {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", archivo);
        return 0;
    }

    cfg->longitudPista = 500;
    cfg->clima = CLIMA_SECO;
    cfg->numAutos = 0;
    cfg->autos = NULL;

    /* Contar autos primero para reservar memoria */
    char linea[256];
    int conteo = 0;
    while (fgets(linea, sizeof(linea), f))
        if (strncmp(linea, "auto=", 5) == 0) conteo++;
    rewind(f);

    /* Reservar arreglo dinámico */
    cfg->autos = (Auto*)malloc(sizeof(Auto) * (conteo > 0 ? conteo : 1));
    if (!cfg->autos) { fclose(f); return 0; }

    int idx = 0;
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') continue;

        if (strncmp(linea, "longitud_pista=", 15) == 0) {
            cfg->longitudPista = atoi(linea + 15);
        }
        else if (strncmp(linea, "clima=", 6) == 0) {
            char cs[16]; sscanf(linea + 6, "%15s", cs);
            if (strcmp(cs, "LLUVIA") == 0) cfg->clima = CLIMA_LLUVIA;
            else if (strcmp(cs, "NIEVE") == 0) cfg->clima = CLIMA_NIEVE;
            else                                 cfg->clima = CLIMA_SECO;
        }
        else if (strncmp(linea, "auto=", 5) == 0 && idx < conteo) {
            Auto* a = &cfg->autos[idx];
            memset(a, 0, sizeof(Auto));
            char tipo; float extra;
            sscanf(linea + 5, "%31[^,],%f,%f,%c,%f",
                a->nombre, &a->velocidadBase, &a->destreza, &tipo, &extra);
            a->tipoVehiculo = tipo;
            if (tipo == TIPO_DEPORTIVO) a->especificaciones.turbo = extra;
            else                        a->especificaciones.traccion = extra;
            a->color = COLORES_AUTO[idx % 8];
            idx++;
        }
    }
    fclose(f);
    cfg->numAutos = idx;
    if (cfg->numAutos > 0) cfg->autos[0].esJugador = 1;
    return 1;
}

/* ════════════════════════════════════════════
   actualizarPosiciones  — avanza un turno
   ════════════════════════════════════════════ */
static float factorClima(Clima clima, char tipo) {
    if (clima == CLIMA_LLUVIA) return (tipo == TIPO_TODOTERRENO) ? 0.95f : 0.80f;
    if (clima == CLIMA_NIEVE)  return (tipo == TIPO_TODOTERRENO) ? 0.85f : 0.60f;
    return 1.0f;
}

void actualizarPosiciones(Configuracion* cfg, int teclaArriba, int teclaAbajo) 
{
    for (int i = 0; i < cfg->numAutos; i++) 
    {
        Auto* a = &cfg->autos[i];
        float posicionAnterior = a->posicion;

        if (a->accidentado) 
        {
            if (--a->turnosAccidente <= 0) a->accidentado = 0;
            a->velocidadActual = 0;
            continue;
        }

        float vel = a->velocidadBase;

        if (a->tipoVehiculo == TIPO_DEPORTIVO)
            vel += a->especificaciones.turbo * 0.05f;
        else
            vel += a->especificaciones.traccion * 0.03f;

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
        }
        else 
        {
            float lider = 0.0f;
            float masCercanoAdelante = 999999.0f;
            int vaUltimo = 1;

            for (int j = 0; j < cfg->numAutos; j++) 
            {
                if (cfg->autos[j].posicion > lider)
                    lider = cfg->autos[j].posicion;

                if (cfg->autos[j].posicion < a->posicion)
                    vaUltimo = 0;

                float dif = cfg->autos[j].posicion - a->posicion;
                if (dif > 0 && dif < masCercanoAdelante)
                    masCercanoAdelante = dif;
            }

            float diferenciaLider = lider - a->posicion;

            if (diferenciaLider > 400) 
            {
                vel *= 1.18f;
            }
            else if (diferenciaLider > 200) 
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
                printf("[IA] %s aprovecho el rebufo para acercarse.\n", a->nombre);
            }

            if (masCercanoAdelante < 35) 
            {
                vel *= 1.18f;
                printf("[IA] %s intento un rebase agresivo.\n", a->nombre);
            }

            if (cfg->clima == CLIMA_LLUVIA && a->destreza < 90) 
                vel *= 0.92f;

            if (cfg->clima == CLIMA_NIEVE && a->tipoVehiculo == TIPO_DEPORTIVO) 
                vel *= 0.88f;

            float estrategia = ((float)rand() / RAND_MAX) * 0.20f - 0.05f;
            vel *= (1.0f + estrategia);
        }//else

        vel *= factorClima(cfg->clima, a->tipoVehiculo);

        if (a->esJugador && usandoNitro) 
        {
            vel *= 2.2f;
            vel += 35.0f;
            printf("[NITRO] %s activo nitro! Velocidad: %.1f\n", a->nombre, vel);
        }

        if (rand() % 100 < 8) 
        {
            int evento = rand() % 3;

            if (evento == 0) 
            {
                vel *= 1.25f;
                printf("[BOOST] %s encontro una recta perfecta!\n", a->nombre);
            }
            else if (evento == 1) 
            {
                vel *= 0.80f;
                printf("[CURVA] %s perdio velocidad en una curva cerrada.\n", a->nombre);
            }
            else 
            {
                vel *= 1.12f;
                printf("[REBASE] %s intento un rebase agresivo.\n", a->nombre);
            }
        }

        if (vel < 5.0f) 
            vel = 5.0f;

        a->velocidadActual = vel;
        a->posicion += vel;

        if (a->velocidadActual > a->velocidadMaxima) 
            a->velocidadMaxima = a->velocidadActual;

        float prob = (100.0f - a->destreza) / 1200.0f;

        if (a->esJugador && usandoNitro) prob *= 1.8f;
        if (!a->esJugador && vel > a->velocidadBase * 1.25f) prob *= 1.4f;
        if (cfg->clima == CLIMA_LLUVIA) prob *= 1.6f;
        if (cfg->clima == CLIMA_NIEVE) prob *= 2.7f;

        if ((float)rand() / RAND_MAX < prob) {
            a->accidentado = 1;
            a->turnosAccidente = 2 + rand() % 3;
            a->accidentesTotales++;

            printf("[!] %s tuvo un accidente! (%d turnos)\n",
                a->nombre, a->turnosAccidente);
        }

        for (int j = 0; j < cfg->numAutos; j++) 
        {
            if (i == j) continue;

            if (posicionAnterior < cfg->autos[j].posicion && a->posicion > cfg->autos[j].posicion) 
            {
                a->adelantamientos++;
            }
        }
    }
}


/* ════════════════════════════════════════════
   determinarGanador
   ════════════════════════════════════════════ */
int determinarGanador(Configuracion* cfg) {
    for (int i = 0; i < cfg->numAutos; i++)
        if (cfg->autos[i].posicion >= cfg->longitudPista) return i;
    return -1;
}

void liberarConfiguracion(Configuracion* cfg) {
    if (cfg->autos) { free(cfg->autos); cfg->autos = NULL; }
    cfg->numAutos = 0;
}

void imprimirConsolaEstado(Configuracion* cfg, int turno) {
    /* Ordenar por posición */
    for (int i = 0; i < cfg->numAutos - 1; i++)
        for (int j = 0; j < cfg->numAutos - 1 - i; j++)
            if (cfg->autos[j].posicion < cfg->autos[j + 1].posicion) {
                Auto tmp = cfg->autos[j];
                cfg->autos[j] = cfg->autos[j + 1];
                cfg->autos[j + 1] = tmp;
            }

    printf("\033[H\033[2J");
    printf("=================================================\n");
    printf("  SIMULADOR F1  —  Turno %-5d\n", turno);
    printf("  Clima: %-8s  Pista: %d unidades\n",
        climaNombre(cfg->clima), cfg->longitudPista);
    printf("=================================================\n");
    printf("  # | %-14s | Posicion | Vel.  | Estado\n", "Piloto");
    printf("-------------------------------------------------\n");
    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        printf(" %2d | %-14s | %7.1f  | %5.1f | %s\n",
            i + 1, a->nombre, a->posicion, a->velocidadActual,
            a->accidentado ? "CRASH" : "OK");
    }
    printf("=================================================\n");
    printf("  [flecha arriba] Acelerar  [flecha abajo] Frenar  [ESC] Salir\n");
    fflush(stdout);
}


static void dibujarAuto(float cx, float cy, ALLEGRO_COLOR color,
    int esJugador, int accidentado,
    const char* inicial, ALLEGRO_FONT* fuente)
{
    ALLEGRO_COLOR c = accidentado ? al_map_rgb(255, 80, 80) : color;
    float w = 36, h = 18;

    al_draw_filled_rounded_rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2, 5, 5, c);
    al_draw_filled_rounded_rectangle(cx - 8, cy - h / 2 - 6, cx + 8, cy - h / 2 + 2, 3, 3,
        al_map_rgba(255, 255, 255, 120));

    ALLEGRO_COLOR rueda = al_map_rgb(30, 30, 30);
    al_draw_filled_circle(cx - w / 2 + 5, cy + h / 2 - 2, 5, rueda);
    al_draw_filled_circle(cx + w / 2 - 5, cy + h / 2 - 2, 5, rueda);

    if (esJugador)
        al_draw_rounded_rectangle(cx - w / 2 - 2, cy - h / 2 - 8, cx + w / 2 + 2, cy + h / 2 + 2,
            6, 6, al_map_rgb(0, 220, 255), 2.0f);
    if (fuente)
        al_draw_text(fuente, al_map_rgb(10, 10, 10), cx, cy - 6,
            ALLEGRO_ALIGN_CENTRE, inicial);
    if (accidentado && fuente)
        al_draw_text(fuente, al_map_rgb(255, 80, 80), cx, cy - h / 2 - 20,
            ALLEGRO_ALIGN_CENTRE, "X");
}

  
void dibujarPista(Configuracion* cfg, ALLEGRO_FONT* fuente, int turno) {
    int n = cfg->numAutos;

    float escala = 2.0f;
    float camara = 0.0f;

    for (int j = 0; j < n; j++) {
        if (cfg->autos[j].esJugador) {
            camara = cfg->autos[j].posicion - 250.0f;
            if (camara < 0) camara = 0;
            break;
        }
    }

    al_draw_filled_rectangle(0, MARGEN_Y - 20,
        ANCHO_VENTANA, MARGEN_Y + n * ALTO_CARRIL + 20,
        al_map_rgb(30, 80, 30));

    for (int i = 0; i < n; i++) {
        float y0 = MARGEN_Y + i * ALTO_CARRIL;
        ALLEGRO_COLOR col = (i % 2 == 0) ? al_map_rgb(50, 50, 65) : al_map_rgb(42, 42, 55);

        al_draw_filled_rectangle(MARGEN_X, y0, MARGEN_X + ANCHO_PISTA, y0 + ALTO_CARRIL, col);

        for (int x = MARGEN_X - ((int)(camara * escala) % 40); x < MARGEN_X + ANCHO_PISTA; x += 40) {
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

    if (xMeta >= MARGEN_X && xMeta <= MARGEN_X + ANCHO_PISTA) {
        for (int k = 0; k < (int)(py1 - py0); k += 10) {
            ALLEGRO_COLOR mc = (k / 10 % 2 == 0) ? al_map_rgb(255, 255, 255) : al_map_rgb(0, 0, 0);
            al_draw_filled_rectangle(xMeta - 5, py0 + k, xMeta + 5, py0 + k + 10, mc);
        }

        if (fuente) {
            al_draw_text(fuente, al_map_rgb(255, 255, 100), xMeta, py0 - 18,
                ALLEGRO_ALIGN_CENTRE, "META");
        }
    }

    if (fuente) {
        al_draw_text(fuente, al_map_rgb(100, 200, 100), MARGEN_X, py0 - 18,
            ALLEGRO_ALIGN_CENTRE, "SALIDA");
    }

    for (int i = 0; i < n; i++) {
        Auto* a = &cfg->autos[i];

        float px = MARGEN_X + (a->posicion - camara) * escala;
        float py = MARGEN_Y + i * ALTO_CARRIL + ALTO_CARRIL / 2.0f;

        if (px >= MARGEN_X - 40 && px <= MARGEN_X + ANCHO_PISTA + 40) {
            char ini[3] = { a->nombre[0], '\0' };
            dibujarAuto(px, py, a->color, a->esJugador, a->accidentado, ini, fuente);
        }
    }

    if (fuente) {
        char buf[128];

        snprintf(buf, sizeof(buf), "Turno: %d", turno);
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
            "Arriba: NITRO  Abajo : frenar  ESC : salir");
    }
}

void dibujarTabla(Configuracion* cfg, ALLEGRO_FONT* fuente) {
    if (!fuente) return;
    int n = cfg->numAutos;
    float ty = MARGEN_Y + n * ALTO_CARRIL + 30;
    float xs = MARGEN_X;
    float cw[5] = { 30,160,120,110,90 };

    const char* hdr[] = { "#","Piloto","Posicion","Vel.","Estado" };
    float x = xs;
    for (int c = 0; c < 5; c++) {
        al_draw_text(fuente, al_map_rgb(255, 200, 0), x + 4, ty, 0, hdr[c]);
        x += cw[c];
    }
    ty += 18;
    al_draw_line(xs, ty, xs + 600, ty, al_map_rgb(120, 120, 60), 1.0f);
    ty += 4;

    //Copia ordenada por posición 
    Auto orden[MAX_AUTOS];
    int cnt = n < MAX_AUTOS ? n : MAX_AUTOS;
    for (int i = 0; i < cnt; i++) orden[i] = cfg->autos[i];
    for (int i = 0; i < cnt - 1; i++)
        for (int j = 0; j < cnt - 1 - i; j++)
            if (orden[j].posicion < orden[j + 1].posicion) {
                Auto tmp = orden[j]; orden[j] = orden[j + 1]; orden[j + 1] = tmp;
            }

    for (int i = 0; i < cnt; i++) {
        Auto* a = &orden[i];
        char buf[32];
        x = xs;
        ALLEGRO_COLOR rc = a->esJugador ? al_map_rgb(0, 220, 255)
            : a->accidentado ? al_map_rgb(255, 80, 80)
            : al_map_rgb(230, 230, 230);

        snprintf(buf, sizeof(buf), "%d", i + 1);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf); x += cw[0];
        al_draw_text(fuente, rc, x + 4, ty, 0, a->nombre); x += cw[1];
        snprintf(buf, sizeof(buf), "%.0f/%d", a->posicion, cfg->longitudPista);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf); x += cw[2];
        snprintf(buf, sizeof(buf), "%.1f u/t", a->velocidadActual);
        al_draw_text(fuente, rc, x + 4, ty, 0, buf); x += cw[3];
        al_draw_text(fuente, rc, x + 4, ty, 0, a->accidentado ? "CRASH" : "OK");
        ty += 20;
    }

    // Barra de progreso del jugador
    for (int i = 0; i < n; i++) {
        if (!cfg->autos[i].esJugador) continue;
        Auto* jug = &cfg->autos[i];
        ty += 8;
        float prog = jug->posicion / (float)cfg->longitudPista;
        if (prog > 1.0f) prog = 1.0f;
        float bw = 600;
        al_draw_text(fuente, al_map_rgb(0, 220, 255), xs, ty, 0, "Tu progreso:");
        ty += 16;
        al_draw_filled_rectangle(xs, ty, xs + bw, ty + 12, al_map_rgb(40, 40, 60));
        al_draw_filled_rectangle(xs, ty, xs + bw * prog, ty + 12, al_map_rgb(0, 220, 255));
        al_draw_rectangle(xs, ty, xs + bw, ty + 12, al_map_rgb(120, 120, 120), 1.0f);
        char pct[16]; snprintf(pct, sizeof(pct), "%.0f%%", prog * 100.0f);
        al_draw_text(fuente, al_map_rgb(10, 10, 10),
            xs + bw * prog / 2.0f, ty, ALLEGRO_ALIGN_CENTRE, pct);
        break;
    }
}

static void dibujarMenu(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal, Configuracion* cfg) {
    for (int y = 0; y < ALTO_VENTANA; y++) {
        float t = (float)y / ALTO_VENTANA;
        al_draw_line(0, y, ANCHO_VENTANA, y,
            al_map_rgb((int)(10 + t * 20), (int)(10 + t * 10), (int)(30 + t * 30)), 1.0f);
    }
    al_draw_text(grande, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 80, ALLEGRO_ALIGN_CENTRE, "SIMULADOR F1");
    al_draw_text(normal, al_map_rgb(200, 200, 200),
        ANCHO_VENTANA / 2, 150, ALLEGRO_ALIGN_CENTRE,
        "Presiona ENTER para iniciar  |  ESC para salir");

    char buf[128];
    snprintf(buf, sizeof(buf), "Pista: %d unidades   |   Clima: %s",
        cfg->longitudPista, climaNombre(cfg->clima));
    al_draw_text(normal, al_map_rgb(150, 220, 150), ANCHO_VENTANA / 2, 200,
        ALLEGRO_ALIGN_CENTRE, buf);
    al_draw_text(normal, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 250,
        ALLEGRO_ALIGN_CENTRE, "--- Pilotos registrados ---");

    for (int i = 0; i < cfg->numAutos; i++) {
        Auto* a = &cfg->autos[i];
        float extra = (a->tipoVehiculo == TIPO_DEPORTIVO)
            ? a->especificaciones.turbo : a->especificaciones.traccion;
        const char* tn = (a->tipoVehiculo == TIPO_DEPORTIVO) ? "Turbo" : "Traccion";
        snprintf(buf, sizeof(buf), "%s%-12s  Vel:%3.0f  Dest:%3.0f  %s:%.0f%s",
            a->esJugador ? ">> " : "   ",
            a->nombre, a->velocidadBase, a->destreza, tn, extra,
            a->esJugador ? " << TU" : "");
        ALLEGRO_COLOR c = a->esJugador ? al_map_rgb(0, 220, 255) : a->color;
        al_draw_text(normal, c, ANCHO_VENTANA / 2, 280 + i * 22, ALLEGRO_ALIGN_CENTRE, buf);
    }

    int cy = 280 + cfg->numAutos * 22 + 30;
    al_draw_text(normal, al_map_rgb(180, 180, 180), ANCHO_VENTANA / 2, cy,
        ALLEGRO_ALIGN_CENTRE,
        "Controles: Arriba=Acelerar  Abajo=Frenar  ESC=Salir");
}

static void dibujarFin(ALLEGRO_FONT* grande, ALLEGRO_FONT* normal,
    Configuracion* cfg, int ganadorIdx, int turno) {
    al_clear_to_color(al_map_rgb(10, 10, 20));
    al_draw_text(grande, al_map_rgb(255, 200, 0),
        ANCHO_VENTANA / 2, 60, ALLEGRO_ALIGN_CENTRE, "FIN DE CARRERA");

    if (ganadorIdx >= 0 && ganadorIdx < cfg->numAutos) {
        char buf[128];
        Auto* g = &cfg->autos[ganadorIdx];
        snprintf(buf, sizeof(buf), "Ganador: %s!", g->nombre);
        ALLEGRO_COLOR gc = g->esJugador ? al_map_rgb(0, 220, 255) : g->color;
        al_draw_text(grande, gc, ANCHO_VENTANA / 2, 140, ALLEGRO_ALIGN_CENTRE, buf);
        snprintf(buf, sizeof(buf), "Completo la carrera en %d turnos", turno);
        al_draw_text(normal, al_map_rgb(200, 200, 200), ANCHO_VENTANA / 2, 210,
            ALLEGRO_ALIGN_CENTRE, buf);
    }

    al_draw_text(normal, al_map_rgb(255, 200, 0), ANCHO_VENTANA / 2, 270,
        ALLEGRO_ALIGN_CENTRE, "--- Clasificacion Final ---");

    Auto orden[MAX_AUTOS];
    int cnt = cfg->numAutos < MAX_AUTOS ? cfg->numAutos : MAX_AUTOS;
    for (int i = 0;i < cnt;i++) orden[i] = cfg->autos[i];
    for (int i = 0;i < cnt - 1;i++)
        for (int j = 0;j < cnt - 1 - i;j++)
            if (orden[j].posicion < orden[j + 1].posicion) {
                Auto tmp = orden[j];orden[j] = orden[j + 1];orden[j + 1] = tmp;
            }

    for (int i = 0;i < cnt;i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d.  %-14s  %.0f unidades",
            i + 1, orden[i].nombre, orden[i].posicion);
        ALLEGRO_COLOR c = orden[i].esJugador ? al_map_rgb(0, 220, 255) : orden[i].color;
        al_draw_text(normal, c, ANCHO_VENTANA / 2, 300 + i * 24, ALLEGRO_ALIGN_CENTRE, buf);
    }

    al_draw_text(normal, al_map_rgb(150, 150, 150), ANCHO_VENTANA / 2, ALTO_VENTANA - 40,
        ALLEGRO_ALIGN_CENTRE,
        "ENTER = volver al menu  |  ESC = salir");
}

// MAIN

int main(void) {
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
    if (!leerConfiguracion("config.txt", &cfg)) 
    {
        fprintf(stderr, "No se pudo leer config.txt\n");
        return 1;
    }
    printf("Cargado: %d autos, pista=%d, clima=%s\n",
        cfg.numAutos, cfg.longitudPista, climaNombre(cfg.clima));

    EstadoJuego estado = ESTADO_MENU;
    int turno = 0, ganadorIdx = -1, salir = 0, redibujar = 0;
    int teclaArriba = 0, teclaAbajo = 0;
    int framesPorTurno = 8, frameContador = 0;

    while (!salir) 
    {
        ALLEGRO_EVENT ev;
        al_wait_for_event(cola, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) salir = 1;

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) 
        {
            switch (ev.keyboard.keycode) {
            case ALLEGRO_KEY_ESCAPE: salir = 1; break;
            case ALLEGRO_KEY_ENTER:
                if (estado == ESTADO_MENU) {
                    for (int i = 0;i < cfg.numAutos;i++) {
                        cfg.autos[i].posicion = 0;
                        cfg.autos[i].velocidadActual = 0;
                        cfg.autos[i].accidentado = 0;
                        cfg.autos[i].turnosAccidente = 0;
                        cfg.autos[i].nitro = cfg.autos[i].esJugador ? 35 : 0;
                        cfg.autos[i].nitroUsado = 0;
                        cfg.autos[i].adelantamientos = 0;
                        cfg.autos[i].accidentesTotales = 0;
                        cfg.autos[i].velocidadMaxima = 0;
                    }
                    turno = 0; ganadorIdx = -1;
                    estado = ESTADO_CORRIENDO;
                }
                else if (estado == ESTADO_FIN) {
                    estado = ESTADO_MENU;
                }
                break;
            case ALLEGRO_KEY_UP:   teclaArriba = 1; break;
            case ALLEGRO_KEY_DOWN: teclaAbajo = 1;  break;
            }
        }
        if (ev.type == ALLEGRO_EVENT_KEY_UP) 
        {
            if (ev.keyboard.keycode == ALLEGRO_KEY_UP)   
                   teclaArriba = 0;
            if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) 
                teclaAbajo = 0;
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redibujar = 1;
            if (estado == ESTADO_CORRIENDO) {
                if (++frameContador >= framesPorTurno) {
                    frameContador = 0;
                    turno++;
                    actualizarPosiciones(&cfg, teclaArriba, teclaAbajo);
                    imprimirConsolaEstado(&cfg, turno);
                    ganadorIdx = determinarGanador(&cfg);
                    if (ganadorIdx >= 0) {
                        printf("\nGanador: %s en %d turnos!\n",
                            cfg.autos[ganadorIdx].nombre, turno);
                        estado = ESTADO_FIN;
                    }
                }
            }
        }

        if (redibujar && al_is_event_queue_empty(cola)) 
        {
            redibujar = 0;
            al_clear_to_color(al_map_rgb(15, 15, 25));
            switch (estado) {
            case ESTADO_MENU:
                dibujarMenu(fGrande, fNormal, &cfg);
                break;
            case ESTADO_CORRIENDO:
                dibujarPista(&cfg, fNormal, turno);
                dibujarTabla(&cfg, fNormal);
                break;
            case ESTADO_FIN:
                dibujarFin(fGrande, fNormal, &cfg, ganadorIdx, turno);
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