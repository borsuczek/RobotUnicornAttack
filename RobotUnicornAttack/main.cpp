#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define UNICORN_BEGIN   SCREEN_WIDTH / 8
#define GROUND_END SCREEN_HEIGHT * 9 / 10
#define UNICORN_END  UNICORN_BEGIN + 90 //90 - szerokoœæ jednoro¿ca w pikselach
#define OBSTACLE_AMOUNT 12


//zerowanie wartoœci aby zresetowaæ grê
void NewGame(double* unicorn_info, double* obstacles_info, double* background_info, double* worldTime, bool* wrap_screen) {
    unicorn_info[0] = 0;
    unicorn_info[1] = 0;
    unicorn_info[2] = 1;
    obstacles_info[0] = 0;
    obstacles_info[1] = 0;
    obstacles_info[2] = -1;
    background_info[0] = 0;
    background_info[1] = 0;
    background_info[2] = -1;
    *worldTime = 0;
    *wrap_screen = false;
}


// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
    SDL_Surface* charset) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
};


// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
};


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for (int i = 0; i < l; i++) {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
};


// rysowanie prostok¹ta o d³ugoœci boków l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
    Uint32 outlineColor, Uint32 fillColor) {

    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);

    for (int i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};


void DrawBackground(SDL_Surface* screen, SDL_Surface* sprite, double* background_info, Uint32 zielony) {
    SDL_Rect dest;
    int x = background_info[1];//przebyty dystans

    x = x % (sprite->w - SCREEN_WIDTH);//zapêt³anie t³a (zawsze czêœæ t³a o d³ugoœci SCREEN_WIDTH musi byæ widoczna)
    dest.x = x * background_info[2];//x * kierunek
    dest.y = 0;

    SDL_BlitSurface(sprite, NULL, screen, &dest);

    DrawRectangle(screen, 0, GROUND_END, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_END, zielony, zielony);
};


void DrawObstacles(SDL_Surface* screen, SDL_Surface* sprite, double* obs_info, int* obstacle_crush, bool* wrap_screen) {
    int distance = obs_info[1], sprite_width = sprite->w, sprite_height = sprite->h, scene_size = 0, i = 0;
    int obssizey[OBSTACLE_AMOUNT] = { 3, 4, 2, 6, 1, 4, 7, 5, 4, 2, 5, 4 }; //wysokoœæ kolejnych przeszkód
    int obssizex[OBSTACLE_AMOUNT] = { 1, 4, 3, 2, 5, 2, 1, 2, 3, 2, 3, 2 }; //d³ugoœæ kolejnych przeszkód
    const int obsinterval = SCREEN_WIDTH / 1.5; //œrednia odleg³oœæ pomiêdzy przeszkodami (przeszkody s¹ ró¿nej d³ugoœci)

    scene_size = (OBSTACLE_AMOUNT)*obsinterval;

    for (int m = 0; m < OBSTACLE_AMOUNT + 1; m++) {//m=0 jest pomocnicze
        SDL_Rect dest;
        int x = distance % (scene_size);//zapêtlanie wyœwietlanych przeszkód

        if (x == scene_size - OBSTACLE_AMOUNT)
            *wrap_screen = true;
        if (m == 0 && *wrap_screen == true) {//pokazanie ostatniej przeszkody gdy ekran siê zawija
            i = OBSTACLE_AMOUNT - 1;
            x += scene_size;
        }
        else {
            i = m - 1;
        }
        //tworzenie przeszkody w zale¿noœci od jej wysokoœci i d³ugoœci
        for (int j = 0; j < obssizey[i]; j++) {
            for (int k = 0; k < obssizex[i]; k++) {
                dest.y = GROUND_END - (j + 1) * sprite_height;
                dest.x = x * obs_info[2] + (SCREEN_WIDTH + (double)obsinterval * i + (k * (double)sprite_width)) * (-obs_info[2]);

                SDL_BlitSurface(sprite, NULL, screen, &dest);
            }
            //sprawdzenie czy przeszkoda znajduje siê w obrêbie jednoro¿ca
            if (dest.x - (obssizex[i] - 1) * sprite_width <= UNICORN_END && dest.x + sprite_width >= UNICORN_BEGIN) {
                obstacle_crush[0] = dest.y;//wysokoœæ przeszkody

                if (dest.x - (obssizex[i] - 1) * sprite_width == UNICORN_END)
                    obstacle_crush[1] = 1;//jednoro¿ec mo¿e uderzyæ w przeszkodê
            }
        }
    }
}


void DrawUnicorn(SDL_Surface* screen, SDL_Surface* sprite, double* uni_info, double delta, int* obstacle_crush, bool* crush) {
    int y, sprite_height = sprite->h;
    double first_y = SCREEN_HEIGHT / 2 - (double)sprite_height / 2;//pocz¹tkowa pozycja jedroro¿ca
    y = first_y + uni_info[1] + uni_info[0] * delta * uni_info[2];//indeks 1 - dystans, 0 - prêdkoœæ, 2 - kierunek

    SDL_Rect dest;
    dest.x = UNICORN_BEGIN;
    //sprawdzenie czy jednoro¿ec uderza w ziemie
    if (y >= GROUND_END - sprite_height && uni_info[2] == 1) {
        dest.y = GROUND_END - sprite_height;
        uni_info[1] = dest.y - first_y;//jednoro¿ec nie startuje ju¿ z pozycji pocz¹tkowej, zostaje na ziemi
    }
    //sprawdzenie czy jednoro¿ec uderza w sufit
    else if (y <= 40 && uni_info[2] == -1) {
        dest.y = 40;
        uni_info[1] = dest.y - first_y;
    }
    else {
        uni_info[1] += uni_info[0] * delta * uni_info[2];//przebyty dystans zwiêksza wartoœæ
        dest.y = y;
    }
    //sprawdzenie czy jednoro¿ec znajduje siê na wysokoœci przeszkody b¹dŸ poni¿ej
    if (obstacle_crush[0] > 0 && dest.y > obstacle_crush[0] - sprite_height) {
        dest.y = obstacle_crush[0] - sprite_height;
        uni_info[1] = dest.y - first_y;//jednoro¿ec zostaje na przeszkodzie
        obstacle_crush[0] = 0;
        //sprawdzenie czy jednoro¿ec uderza w przeszkodê
        if (obstacle_crush[1] == 1) {
            obstacle_crush[1] = 0;
            *crush = true;
        }
    }
    obstacle_crush[1] = 0;

    SDL_BlitSurface(sprite, NULL, screen, &dest);
}


// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
    int t1, t2, quit, rc;
    //indeks 0 - wysokoœæ przeszkody w obrêbie jednoro¿ca, indeks 1 - czy jednoro¿ec mo¿e ude¿yæ w przeszkodê (1 - tak, 0 - nie)
    int obstacle_crush[2];
    //odpowiednio informacja o szybkoœci, dystansie i kierunku poruszania dla jednoro¿ca, przeszkód oraz t³a
    double unicorn_info[3], obstacles_info[3], background_info[3];
    double delta, worldTime;
    bool n_is_active = false, wrap_screen = false, crush = false;
    SDL_Event event;
    SDL_Surface* screen, * charset;
    SDL_Surface* background, * obstacle, * unicorn;
    SDL_Texture* scrtex;
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    // tryb pe³noekranowy / fullscreen mode
    //rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
                                     //&window, &renderer);
    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
        &window, &renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    };

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "Szablon do zdania drugiego 2017");


    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);


    // wy³¹czenie widocznoœci kursora myszy
    SDL_ShowCursor(SDL_DISABLE);

    // wczytanie obrazków
    charset = SDL_LoadBMP("./cs8x8.bmp");
    if (charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    };
    SDL_SetColorKey(charset, true, 0x000000);

    background = SDL_LoadBMP("./tlo.bmp");
    if (background == NULL) {
        printf("SDL_LoadBMP(tlo.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    };

    obstacle = SDL_LoadBMP("./przeszkoda.bmp");
    if (background == NULL) {
        printf("SDL_LoadBMP(przeszkoda.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    };

    unicorn = SDL_LoadBMP("./unicorn2.bmp");
    if (background == NULL) {
        printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    };

    char text[128];
    int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

    t1 = SDL_GetTicks();

    quit = 0;

    NewGame(unicorn_info, obstacles_info, background_info, &worldTime, &wrap_screen);

    while (!quit) {
        t2 = SDL_GetTicks();

        // w tym momencie t2-t1 to czas w milisekundach,
        // jaki uplyna³ od ostatniego narysowania ekranu
        // delta to ten sam czas w sekundach
        delta = (t2 - t1) * 0.001;
        t1 = t2;

        worldTime += delta;

        //dystans += szybkoœæ * delta
        background_info[1] += background_info[0] * delta;
        obstacles_info[1] += obstacles_info[0] * delta;

        SDL_FillRect(screen, NULL, czarny);
        DrawBackground(screen, background, background_info, zielony);
        DrawObstacles(screen, obstacle, obstacles_info, obstacle_crush, &wrap_screen);
        DrawUnicorn(screen, unicorn, unicorn_info, delta, obstacle_crush, &crush);

        //sprawdzenie czy jednoro¿ec wpad³ w przeszkodê, jeœli tak - reset gry
        if (crush == true) {
            NewGame(unicorn_info, obstacles_info, background_info, &worldTime, &wrap_screen);
            crush = false;
        }

        // tekst informacyjny
        DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
        sprintf(text, "Robot Unicorn Attack, time = %.1lf s", worldTime);
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
        sprintf(text, "Esc - escape, n - new game, \030 - up, \031 - down, \033 - right");
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        //              SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        // obs³uga zdarzeñ (o ile jakieœ zasz³y)
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                else if (event.key.keysym.sym == SDLK_UP) { unicorn_info[2] = -1; unicorn_info[0] = 150; }
                else if (event.key.keysym.sym == SDLK_DOWN) { unicorn_info[2] = 1; unicorn_info[0] = 150; }
                else if (event.key.keysym.sym == SDLK_RIGHT) { background_info[0] = 60; obstacles_info[0] = 300; }
                else if (event.key.keysym.sym == SDLK_n && n_is_active == false) {
                    NewGame(unicorn_info, obstacles_info, background_info, &worldTime, &wrap_screen);
                    n_is_active = true;
                }
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_UP) unicorn_info[0] = 0;
                else if (event.key.keysym.sym == SDLK_DOWN) unicorn_info[0] = 0;
                else if (event.key.keysym.sym == SDLK_RIGHT) { background_info[0] = 0; obstacles_info[0] = 0; }
                else if (event.key.keysym.sym == SDLK_n) n_is_active = false;
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            };
        };
    };

    // zwolnienie powierzchni / freeing all surfaces
    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
};