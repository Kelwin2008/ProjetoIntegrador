#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// Configurações de Tela
#define LARGURA_TELA 800
#define ALTURA_TELA 600

// Estados do Jogo
typedef enum {
    ESTADO_MAPA,
    ESTADO_DECISAO,
    ESTADO_VITORIA,
    ESTADO_GAME_OVER
} EstadoJogo;

// Estrutura de Recursos do Jogador
typedef struct {
    int suporte;
    int movimentos;
    int pontos_resolvidos;
    EstadoJogo estado;
} Jogo;

// Função para reiniciar/inicializar a POC
void reiniciar_jogo(Jogo* j) {
    j->suporte = 3;
    j->movimentos = 10;
    j->pontos_resolvidos = 0;
    j->estado = ESTADO_MAPA;
}

int main() {
    // 1. Inicialização do Allegro e Addons
    if (!al_init()) return -1;
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_mouse();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_FONT* fonte = al_create_builtin_font();

    // Registrar Fontes de Eventos
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());

    // 2. Carregar as Imagens com os nomes exatos do teu projeto
// 2. Carregar as Imagens com validação individual
    ALLEGRO_BITMAP* img_mapa = al_load_bitmap("mapa_hud.png");
    if (!img_mapa) printf("Erro ao carregar: mapa_hud.png\n");

    ALLEGRO_BITMAP* img_decisao = al_load_bitmap("caixa_decisao.png");
    if (!img_decisao) printf("Erro ao carregar: caixa_decisao.png\n");

    ALLEGRO_BITMAP* img_vitoria = al_load_bitmap("vitoria.png");
    if (!img_vitoria) printf("Erro ao carregar: vitoria.png\n");

    ALLEGRO_BITMAP* img_game_over = al_load_bitmap("derrota.png");
    if (!img_game_over) printf("Erro ao carregar: derrota.png\n");

    // Se qualquer uma falhar, encerra
    if (!img_mapa || !img_decisao || !img_vitoria || !img_game_over) {
        system("pause"); // Trava o terminal aberto para você ler qual arquivo falhou
        return -1;
    }

    // Validação de carregamento dos ficheiros
    if (!img_mapa || !img_decisao || !img_vitoria || !img_game_over) {
        printf("Erro: Nao foi possivel carregar uma ou mais imagens!\n");
        return -1;
    }

    Jogo jogo;
    reiniciar_jogo(&jogo);

    bool rodando = true;

    // Loop Principal
    while (rodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        // Evento de Fechar Janela
        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }

        // --- TRATAMENTO DE ENTRADAS (RATO E TECLADO) ---
        if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = evento.mouse.x;
            int my = evento.mouse.y;

            if (jogo.estado == ESTADO_MAPA) {
                // Clique nos Pontos do Mapa
                if ((mx >= 250 && mx <= 350 && my >= 250 && my <= 350) ||
                    (mx >= 500 && mx <= 600 && my >= 300 && my <= 400)) {
                    jogo.estado = ESTADO_DECISAO;
                }
            }
            else if (jogo.estado == ESTADO_DECISAO) {
                // Clique no Botão 1: Investigar (-2 Movimentos)
                if (mx >= 200 && mx <= 600 && my >= 260 && my <= 300) {
                    jogo.movimentos -= 2;
                    jogo.pontos_resolvidos++;
                    jogo.estado = ESTADO_MAPA;
                }
                // Clique no Botão 2: Agir Direto (-1 Suporte)
                else if (mx >= 200 && mx <= 600 && my >= 320 && my <= 360) {
                    jogo.suporte -= 1;
                    jogo.estado = ESTADO_MAPA;
                }

                // Verificar Condições de Fim de Jogo
                if (jogo.suporte <= 0 || jogo.movimentos <= 0) {
                    jogo.estado = ESTADO_GAME_OVER;
                }
                else if (jogo.pontos_resolvidos >= 2) {
                    jogo.estado = ESTADO_VITORIA;
                }
            }
        }

        // Reiniciar com ENTER
        if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                if (jogo.estado == ESTADO_VITORIA || jogo.estado == ESTADO_GAME_OVER) {
                    reiniciar_jogo(&jogo);
                }
            }
        }

        // --- RENDERIZAÇÃO NA TELA ---
        al_clear_to_color(al_map_rgb(0, 0, 0));

        if (jogo.estado == ESTADO_MAPA || jogo.estado == ESTADO_DECISAO) {
            al_draw_bitmap(img_mapa, 0, 0, 0);

            // Desenhar os valores na HUD
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 140, 25, 0, "%d", jogo.suporte);
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 680, 25, 0, "%d", jogo.movimentos);

            if (jogo.estado == ESTADO_DECISAO) {
                al_draw_bitmap(img_decisao, 150, 160, 0);
            }
        }
        else if (jogo.estado == ESTADO_VITORIA) {
            al_draw_bitmap(img_vitoria, 0, 0, 0);
        }
        else if (jogo.estado == ESTADO_GAME_OVER) {
            al_draw_bitmap(img_game_over, 0, 0, 0);
        }

        al_flip_display();
    }

    // Limpeza de Memória
    al_destroy_bitmap(img_mapa);
    al_destroy_bitmap(img_decisao);
    al_destroy_bitmap(img_vitoria);
    al_destroy_bitmap(img_game_over);
    al_destroy_font(fonte);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}