#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>  

#define LARGURA_TELA 800
#define ALTURA_TELA 600
#define FPS 60

// quantos pixels o personagem anda por frame
#define VELOCIDADE 4

// tamanho do personagem (1.0 = original)
#define ESCALA_PERSONAGEM 0.25f

// quantidade de fases do mapa (por enquanto)
#define NUM_FASES 2

// struct pra cada estado do jogo (mapa, andando, cena, decisao, vitoria, game over)
typedef enum {
    ESTADO_MAPA,
    ESTADO_ANDANDO,
    ESTADO_CENA,     
    ESTADO_DECISAO,   
    ESTADO_VITORIA,
    ESTADO_GAME_OVER
} EstadoJogo;

// struct pra oq contém em cada fase
typedef struct {
    int pino_x, pino_y;        // coordenada do pin
    const char* titulo;        // titulo da cena
    const char* texto;         // texto de contexto
    const char* pergunta;      // pergunta feita depois da cena
    const char* opcao1;
    const char* opcao2;
    int certa;                 // 1 ou 2: qual opção e a correta
    ALLEGRO_BITMAP* cena;      // arte da cena (em precesso ainda, esta finalizando)
} Fase;

// struct pras informacoes que mudam durante o jogo
typedef struct {
    int suporte;
    int movimentos;
    int pontos_resolvidos;
    float personagem_x;
    float personagem_y;        // agora o Y tambem muda (adicionamos mais uma fase, e como esta embaixo colocamos a reta Y)
    int fase_atual;            // qual fase foi clicada
    bool resolvida[NUM_FASES]; // true se a pergunta daquela fase ja foi acertada
    EstadoJogo estado;
} Jogo;



// apanhei e recorri pra I.A kkkk
static bool clique_no_retangulo(int mx, int my, int x, int y, int largura, int altura) {
    return (mx >= x && mx <= x + largura && my >= y && my <= y + altura);
} 

// recebe a pontuação inicial do personagem (nós fizemos)
void reiniciar_jogo(Jogo* j, float inicio_x, float inicio_y) {
    j->suporte = 3;
    j->movimentos = 10;
    j->pontos_resolvidos = 0;
    j->personagem_x = inicio_x;
    j->personagem_y = inicio_y;
    j->fase_atual = 0;
    for (int i = 0; i < NUM_FASES; i++) {
        j->resolvida[i] = false;
    }
    j->estado = ESTADO_MAPA;
}

// aproxima um valor do destino, no maximo "passo" por vez (sem passar do destino) I.A
float aproximar(float atual, float destino, float passo) {
    if (atual < destino) {
        atual += passo;
        if (atual > destino) atual = destino;
    }
    else if (atual > destino) {
        atual -= passo;
        if (atual < destino) atual = destino;
    }
    return atual;
}

// NOVO: os numeros do HUD (antes estavam repetidos em varios lugares) I.A
void desenhar_hud(ALLEGRO_FONT* fonte, Jogo* j) {
    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 255, 35, ALLEGRO_ALIGN_CENTER, "%d", j->suporte);
    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 715, 35, ALLEGRO_ALIGN_CENTER, "%d", j->movimentos);
}

// Funcao auxiliar para carregar e otimizar bitmaps na memoria de video I.A
ALLEGRO_BITMAP* carregar_e_otimizar_bitmap(const char* caminho) {
    ALLEGRO_BITMAP* bmp = al_load_bitmap(caminho);
    if (bmp) {
        al_convert_bitmap(bmp);
    }
    return bmp;
}

int main() {
    if (!al_init()) return -1;

    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST); // (I.A)

    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();   
    al_install_mouse();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* fonte = al_create_builtin_font();

    if (!display || !queue || !timer || !fonte) return -1;

    ALLEGRO_FONT* fonte_texto = al_load_ttf_font("C:/Windows/Fonts/arial.ttf", 16, 0); 
    if (!fonte_texto) fonte_texto = fonte;

    ALLEGRO_FONT* fonte_titulo = al_load_ttf_font("C:/Windows/Fonts/arialbd.ttf", 22, 0);
    if (!fonte_titulo) fonte_titulo = fonte;

    // imagens do jogo obrigatorias ( nós fizemos)
    ALLEGRO_BITMAP* img_mapa = carregar_e_otimizar_bitmap("mapa_hud.png");
    ALLEGRO_BITMAP* img_vitoria = carregar_e_otimizar_bitmap("vitoria.png");
    ALLEGRO_BITMAP* img_derrota = carregar_e_otimizar_bitmap("derrota.png");
    ALLEGRO_BITMAP* img_personagem = carregar_e_otimizar_bitmap("personagem.jpg"); // precisamos colocar em PNG

    // mostra exatamente qual arquivo falhou (nós fizemos)
    if (!img_mapa)       printf("Falhou: mapa_hud.png\n");
    if (!img_vitoria)    printf("Falhou: vitoria.png\n");
    if (!img_derrota)    printf("Falhou: derrota.png\n");
    if (!img_personagem) printf("Falhou: personagem.jpg\n"); // precisamos colocar em PNG

    if (!img_mapa || !img_vitoria || !img_derrota || !img_personagem) {
        system("pause");
        return -1;
    }

    //  NOVO: lista de fases
    //  ordem de cada linha: pino_x, pino_y, titulo, texto, pergunta, opcao1, opcao2, certa, cena
    Fase fases[NUM_FASES] = {
        {
            320, 238,
            "Invasão da Polônia (1939)",
            "Em 1º de setembro de 1939, a Alemanha nazista invadiu a Polônia. "
            "Dois dias depois, o Reino Unido e a França declararam guerra à Alemanha. "
            "Esse foi o início da Segunda Guerra Mundial na Europa.",
            "O que a invasão da Polônia desencadeou?",
            "O início da Segunda Guerra Mundial",
            "O fim da Primeira Guerra Mundial",
            1,
            NULL
        },
        {
            390, 422,
            "Desembarque na Sicília (1943)",
            "Em julho de 1943, tropas aliadas desembarcaram na Sicília, na Itália. "
            "A invasão enfraqueceu o governo fascista, e Mussolini foi afastado do poder "
            "poucas semanas depois.",
            "Qual foi uma consequência política do desembarque na Sicília?",
            "A Itália invadiu a França",
            "Mussolini foi afastado do poder",
            2,
            NULL
        }
    };

    // artes das cenas (se o arquivo nao existir, fica NULL e aparece um quadro cinza)
    fases[0].cena = carregar_e_otimizar_bitmap("cena1.png");
    fases[1].cena = carregar_e_otimizar_bitmap("cena2.png");

    // tamanho e posicao do personagem 
    int original_w = al_get_bitmap_width(img_personagem);
    int original_h = al_get_bitmap_height(img_personagem);

    int personagem_w = original_w * ESCALA_PERSONAGEM;
    int personagem_h = original_h * ESCALA_PERSONAGEM;

    // comeca no canto esquerdo, na altura da primeira fase
    float inicio_x = 0;
    float inicio_y = fases[0].pino_y - personagem_h / 2;

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));

    // (nós fizemos, mas a i.a foi auxiliando)
    Jogo jogo;
    reiniciar_jogo(&jogo, inicio_x, inicio_y);

    bool rodando = true;
    bool redesenhar = true;

    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            redesenhar = true;

            // ALTERADO: anda primeiro na horizontal, depois na vertical (sempre em linha reta)
            if (jogo.estado == ESTADO_ANDANDO) {
                float destino_x = fases[jogo.fase_atual].pino_x - personagem_w / 2;
                float destino_y = fases[jogo.fase_atual].pino_y - personagem_h / 2;

                if (jogo.personagem_x != destino_x) {
                    jogo.personagem_x = aproximar(jogo.personagem_x, destino_x, VELOCIDADE);
                }
                else if (jogo.personagem_y != destino_y) {
                    jogo.personagem_y = aproximar(jogo.personagem_y, destino_y, VELOCIDADE);
                }
                else {
                    // chegou: mostra a cena de contexto
                    jogo.estado = ESTADO_CENA;
                }
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = evento.mouse.x;
            int my = evento.mouse.y;

            if (jogo.estado == ESTADO_MAPA) {
                // ALTERADO: testa todos os pins que ainda nao foram resolvidos
                for (int i = 0; i < NUM_FASES; i++) {
                    if (!jogo.resolvida[i] &&
                        clique_no_retangulo(mx, my, fases[i].pino_x - 20, fases[i].pino_y - 28, 40, 56)) {
                        jogo.fase_atual = i;
                        jogo.estado = ESTADO_ANDANDO;
                    }
                }
            }
            else if (jogo.estado == ESTADO_CENA) {
                // NOVO: clicar em qualquer lugar passa da cena para a pergunta
                jogo.estado = ESTADO_DECISAO;
            }
            else if (jogo.estado == ESTADO_DECISAO) {
                int resposta = 0;

                if (clique_no_retangulo(mx, my, 200, 300, 400, 45)) resposta = 1;
                else if (clique_no_retangulo(mx, my, 200, 360, 400, 45)) resposta = 2;

                // ALTERADO: so confere o resultado se clicou em uma das opcoes
                if (resposta != 0) {
                    jogo.movimentos -= 1;

                    if (resposta == fases[jogo.fase_atual].certa) {
                        jogo.resolvida[jogo.fase_atual] = true;
                        jogo.pontos_resolvidos++;
                    }
                    else {
                        jogo.suporte -= 1;
                    }

                    jogo.estado = ESTADO_MAPA;

                    if (jogo.pontos_resolvidos >= NUM_FASES) {
                        jogo.estado = ESTADO_VITORIA;
                    }
                    else if (jogo.suporte <= 0 || jogo.movimentos <= 0) {
                        jogo.estado = ESTADO_GAME_OVER;
                    }
                }
            }
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                if (jogo.estado == ESTADO_CENA) {
                    jogo.estado = ESTADO_DECISAO;
                }
                else if (jogo.estado == ESTADO_VITORIA || jogo.estado == ESTADO_GAME_OVER) {
                    reiniciar_jogo(&jogo, inicio_x, inicio_y);
                }
            }
        }

        //  Renderizacao (I.A)
        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;
            al_clear_to_color(al_map_rgb(0, 0, 0));

            ALLEGRO_COLOR dourado = al_map_rgb(212, 175, 55);
            ALLEGRO_COLOR branco = al_map_rgb(255, 255, 255);

            if (jogo.estado == ESTADO_MAPA || jogo.estado == ESTADO_ANDANDO) {
                al_draw_bitmap(img_mapa, 0, 0, 0);

                al_draw_scaled_bitmap(img_personagem, 0, 0, original_w, original_h,
                    jogo.personagem_x, jogo.personagem_y, personagem_w, personagem_h, 0);

                desenhar_hud(fonte, &jogo);
            }
            else if (jogo.estado == ESTADO_CENA) {
                // fundo: mapa escurecido
                al_draw_bitmap(img_mapa, 0, 0, 0);
                al_draw_filled_rectangle(0, 0, LARGURA_TELA, ALTURA_TELA, al_map_rgba(0, 0, 0, 215));

                // arte da cena (ou quadro cinza se ainda nao existe)
                if (fases[jogo.fase_atual].cena) {
                    int cw = al_get_bitmap_width(fases[jogo.fase_atual].cena);
                    int ch = al_get_bitmap_height(fases[jogo.fase_atual].cena);
                    al_draw_scaled_bitmap(fases[jogo.fase_atual].cena, 0, 0, cw, ch, 100, 60, 600, 300, 0);
                }
                else {
                    al_draw_filled_rectangle(100, 60, 700, 360, al_map_rgb(70, 70, 70));
                    al_draw_text(fonte_texto, branco, 400, 200, ALLEGRO_ALIGN_CENTER, "(arte da cena aqui)");
                }
                al_draw_rectangle(100, 60, 700, 360, dourado, 3);

                // titulo e texto de contexto
                al_draw_text(fonte_titulo, dourado, 400, 378, ALLEGRO_ALIGN_CENTER,
                    fases[jogo.fase_atual].titulo);

                al_draw_multiline_text(fonte_texto, branco, 400, 420, 560,
                    al_get_font_line_height(fonte_texto) + 6, ALLEGRO_ALIGN_CENTER,
                    fases[jogo.fase_atual].texto);

                al_draw_text(fonte_texto, al_map_rgb(170, 170, 170), 400, 560, ALLEGRO_ALIGN_CENTER,
                    "Clique para continuar");
            }
            else if (jogo.estado == ESTADO_DECISAO) {
                al_draw_bitmap(img_mapa, 0, 0, 0);

                al_draw_scaled_bitmap(img_personagem, 0, 0, original_w, original_h,
                    jogo.personagem_x, jogo.personagem_y, personagem_w, personagem_h, 0);

                al_draw_filled_rectangle(0, 0, LARGURA_TELA, ALTURA_TELA, al_map_rgba(0, 0, 0, 170));

                desenhar_hud(fonte, &jogo);

                // painel da pergunta
                al_draw_filled_rectangle(150, 120, 650, 480, al_map_rgb(20, 28, 50));
                al_draw_rectangle(150, 120, 650, 480, dourado, 3);

                al_draw_multiline_text(fonte_titulo, branco, 400, 150, 440, 30,
                    ALLEGRO_ALIGN_CENTER, fases[jogo.fase_atual].pergunta);

                // botao 1 (mesma area de clique de antes: 200,300,400,45)
                al_draw_filled_rectangle(200, 300, 600, 345, al_map_rgb(40, 60, 110));
                al_draw_rectangle(200, 300, 600, 345, dourado, 2);
                al_draw_text(fonte_texto, branco, 400, 300 + (45 - al_get_font_line_height(fonte_texto)) / 2,
                    ALLEGRO_ALIGN_CENTER, fases[jogo.fase_atual].opcao1);

                // botao 2 (200,360,400,45)
                al_draw_filled_rectangle(200, 360, 600, 405, al_map_rgb(40, 60, 110));
                al_draw_rectangle(200, 360, 600, 405, dourado, 2);
                al_draw_text(fonte_texto, branco, 400, 360 + (45 - al_get_font_line_height(fonte_texto)) / 2,
                    ALLEGRO_ALIGN_CENTER, fases[jogo.fase_atual].opcao2);
            }
            else if (jogo.estado == ESTADO_VITORIA) {
                al_draw_bitmap(img_vitoria, 0, 0, 0);
            }
            else if (jogo.estado == ESTADO_GAME_OVER) {
                al_draw_bitmap(img_derrota, 0, 0, 0);
            }

            al_flip_display();
        }
    }

    // Limpeza (nós fizemos)
    al_destroy_bitmap(img_mapa);
    al_destroy_bitmap(img_vitoria);
    al_destroy_bitmap(img_derrota);
    al_destroy_bitmap(img_personagem);

    // NOVO: libera as artes das cenas (so as que existem)
    for (int i = 0; i < NUM_FASES; i++) {
        if (fases[i].cena) al_destroy_bitmap(fases[i].cena);
    }

    // NOVO: so destroi as fontes TTF se elas nao forem a fonte embutida
    if (fonte_texto != fonte) al_destroy_font(fonte_texto);
    if (fonte_titulo != fonte) al_destroy_font(fonte_titulo);
    al_destroy_font(fonte);

    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
