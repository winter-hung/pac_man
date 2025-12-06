// === Pacman with mouth animation & centered ghosts ===
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <cmath>
#include <cstdlib>
#include <ctime>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int TILE_SIZE = 32;

const int PACMAN_RADIUS = 14;
const float PACMAN_SPEED = 120.0f;

const int DOT_RADIUS = 4;

const int GHOST_RADIUS = 14;
const float GHOST_SPEED = 90.0f;

const float PI = 3.1415926535f;

// 嘴巴動畫參數
const float MOUTH_MAX_ANGLE = 0.7f;   // 最大張開（弧度）
const float MOUTH_MIN_ANGLE = 0.1f;   // 最小張開
const float MOUTH_SPEED     = 4.0f;   // 開合速度

struct Vec2 { float x, y; };

// 0=空, 1=牆, 2=豆
const std::vector<std::string> MAP = {
    "1111111111111111111111",
    "1222222222112222222221",
    "1211112111112111112121",
    "1212212122222121212121",
    "1211112121112121111111",  // ← 封住右邊通道
    "1222222122112122222111",  // ← 封住右邊通道
    "1211112111122111111111",  // ← 封住右邊通道
    "1212212122222222212121",
    "1212112121111112112121",
    "1212222122222222122221",
    "1211112111112111112121",
    "1222222222112222222221",
    "1111111111111111111111"
};

const int MAP_WIDTH = MAP[0].size();
const int MAP_HEIGHT = MAP.size();

// =============== 工具函數 =================
bool isWall(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return true;
    return MAP[y][x] == '1';
}

bool circleHitCircle(float x1, float y1, float r1, float x2, float y2, float r2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx * dx + dy * dy <= (r1 + r2) * (r1 + r2);
}

bool tileWalkable(float x, float y)
{
    int gx = (int)(x / TILE_SIZE);
    int gy = (int)(y / TILE_SIZE);
    return !isWall(gx, gy);
}

// 只畫實心圓（給鬼、豆子用）
void drawCircle(SDL_Renderer* renderer, int cx,int cy,int r)
{
    for(int dy=-r;dy<=r;dy++)
        for(int dx=-r;dx<=r;dx++)
            if(dx*dx+dy*dy<=r*r)
                SDL_RenderDrawPoint(renderer,cx+dx,cy+dy);
}

// 畫有嘴巴的 Pac-Man
void drawPacmanMouth(SDL_Renderer* renderer, int cx, int cy, int r, Vec2 dir, float mouthAngle)
{
    // 依照方向決定面向角度：以「向右」為 0
    float facingAngle = 0.0f;
    if (dir.x > 0.5f)        facingAngle = 0.0f;          // 右
    else if (dir.x < -0.5f)  facingAngle = PI;            // 左
    else if (dir.y < -0.5f)  facingAngle = -PI / 2.0f;    // 上
    else if (dir.y > 0.5f)   facingAngle =  PI / 2.0f;    // 下
    else                     facingAngle = 0.0f;          // 靜止時預設朝右

    // 掃整個圓，只畫「不在嘴巴範圍」的點
    for (int y = -r; y <= r; ++y) {
        for (int x = -r; x <= r; ++x) {
            float dist2 = x * x + y * y;
            if (dist2 > r * r) continue;

            float ang = std::atan2((float)y, (float)x); // -pi ~ pi

            // 把角度轉成以 facingAngle 為中心的差值 (-pi ~ pi)
            float diff = ang - facingAngle;
            while (diff >  PI) diff -= 2.0f * PI;
            while (diff < -PI) diff += 2.0f * PI;

            // 嘴巴開口範圍內就空出來
            if (diff > -mouthAngle && diff < mouthAngle)
                continue;

            SDL_RenderDrawPoint(renderer, cx + x, cy + y);
        }
    }
}

// ================ Main =======================
int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow(
        "Pacman Fixed",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    srand((unsigned)time(nullptr));

    // Pac-Man 起始位置在格子中心
    Vec2 pacPos = { 1*TILE_SIZE + TILE_SIZE/2.0f,
                    6*TILE_SIZE  + TILE_SIZE/2.0f };
    Vec2 pacDir = { 0,0 };

    // 嘴巴動畫狀態
    float mouthAngle   = MOUTH_MIN_ANGLE;
    bool  mouthOpening = true;

    struct Ghost {
        Vec2 pos;
        Vec2 dir;
        SDL_Color color;
    };

    std::vector<Ghost> ghosts = {
        {{5*TILE_SIZE+TILE_SIZE/2.0f, 5*TILE_SIZE+TILE_SIZE/2.0f}, { 1, 0}, {255, 0,   0,   255}},
        {{16*TILE_SIZE+TILE_SIZE/2.0f,5*TILE_SIZE+TILE_SIZE/2.0f}, {-1, 0}, {255,170,255, 255}},
        {{5*TILE_SIZE+TILE_SIZE/2.0f, 8*TILE_SIZE+TILE_SIZE/2.0f}, { 0,-1}, {0,  255,255, 255}},
        {{16*TILE_SIZE+TILE_SIZE/2.0f,8*TILE_SIZE+TILE_SIZE/2.0f}, { 0, 1}, {255,180,0,   255}},
    };

    int totalDots = 0;
    for (int y=0;y<MAP_HEIGHT;y++)
        for (int x=0;x<MAP_WIDTH;x++)
            if (MAP[y][x]=='2') totalDots++;

    std::unordered_set<int> eaten;

    Uint64 last = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();

    bool running=true;
    bool gameOver = false;

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (now-last)/freq;
        last = now;

        // ====== 輸入 ======
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type==SDL_QUIT) running=false;
            if (e.type==SDL_KEYDOWN)
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_UP:    pacDir={0,-1}; break;
                    case SDLK_DOWN:  pacDir={0, 1}; break;
                    case SDLK_LEFT:  pacDir={-1,0}; break;
                    case SDLK_RIGHT: pacDir={1, 0}; break;
                }
            }
        }

        // ====== 嘴巴動畫更新 ======
        if (mouthOpening) {
            mouthAngle += MOUTH_SPEED * dt;
            if (mouthAngle > MOUTH_MAX_ANGLE) {
                mouthAngle = MOUTH_MAX_ANGLE;
                mouthOpening = false;
            }
        } else {
            mouthAngle -= MOUTH_SPEED * dt;
            if (mouthAngle < MOUTH_MIN_ANGLE) {
                mouthAngle = MOUTH_MIN_ANGLE;
                mouthOpening = true;
            }
        }

        if (!gameOver)
        {
            // === Pac-Man 移動（靠中心、tile 碰撞）====
            float nx = pacPos.x + pacDir.x * PACMAN_SPEED * dt;
            float ny = pacPos.y + pacDir.y * PACMAN_SPEED * dt;

            // 讓 Pac-Man 在水平移動時自動吸回 Y 中心
            if (pacDir.x != 0) {
                int gy = (int)(pacPos.y / TILE_SIZE);
                pacPos.y = gy * TILE_SIZE + TILE_SIZE / 2.0f;
            }

            // 在垂直移動時吸回 X 中心
            if (pacDir.y != 0) {
                int gx = (int)(pacPos.x / TILE_SIZE);
                pacPos.x = gx * TILE_SIZE + TILE_SIZE / 2.0f;
            }

            if (tileWalkable(nx, pacPos.y)) pacPos.x = nx;
            if (tileWalkable(pacPos.x, ny)) pacPos.y = ny;

            // === 吃豆子 ===
            int gx = (int)(pacPos.x / TILE_SIZE);
            int gy = (int)(pacPos.y / TILE_SIZE);

            if (gx>=0 && gx<MAP_WIDTH && gy>=0 && gy<MAP_HEIGHT)
            {
                if (MAP[gy][gx]=='2')
                {
                    int idx = gy*MAP_WIDTH + gx;
                    if (!eaten.count(idx))
                        eaten.insert(idx);
                }
            }

            if ((int)eaten.size() == totalDots)
                gameOver=true;

            // ===== 鬼移動（走在路中間） =====
            for (auto &g : ghosts)
            {
                // 鎖中心：若鬼水平走，就讓 y 在所在格子的中心，反之亦然
                if (std::fabs(g.dir.x) > 0.5f) {
                    int gy = (int)(g.pos.y / TILE_SIZE);
                    g.pos.y = gy * TILE_SIZE + TILE_SIZE/2.0f;
                }
                if (std::fabs(g.dir.y) > 0.5f) {
                    int gx2 = (int)(g.pos.x / TILE_SIZE);
                    g.pos.x = gx2 * TILE_SIZE + TILE_SIZE/2.0f;
                }

                float nxg = g.pos.x + g.dir.x * GHOST_SPEED * dt;
                float nyg = g.pos.y + g.dir.y * GHOST_SPEED * dt;

                bool blocked = false;
                if (!tileWalkable(nxg, g.pos.y)) blocked=true;
                if (!tileWalkable(g.pos.x, nyg)) blocked=true;

                if (!blocked)
                {
                    g.pos.x = nxg;
                    g.pos.y = nyg;
                }
                else
                {
                    // 碰牆時隨機換一個可走方向
                    std::vector<Vec2> dirs = {{1,0},{-1,0},{0,1},{0,-1}};
                    std::vector<Vec2> valid;

                    for (auto &d:dirs)
                    {
                        // 往該方向一半 tile 試試看
                        float tx = g.pos.x + d.x * TILE_SIZE/2.0f;
                        float ty = g.pos.y + d.y * TILE_SIZE/2.0f;
                        if (tileWalkable(tx,ty))
                            valid.push_back(d);
                    }

                    if (!valid.empty())
                        g.dir = valid[rand()%valid.size()];
                }

                // 抓到 Pac-Man ?
                if (circleHitCircle(pacPos.x,pacPos.y,PACMAN_RADIUS, g.pos.x,g.pos.y,GHOST_RADIUS))
                    gameOver=true;
            }
        }

        // =============== Render =================
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);

        // 繪地圖與豆子
        for(int y=0;y<MAP_HEIGHT;y++)
        for(int x=0;x<MAP_WIDTH;x++)
        {
            SDL_Rect r={x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE};

            if (MAP[y][x]=='1')
            {
                SDL_SetRenderDrawColor(renderer,0,0,255,255);
                SDL_RenderFillRect(renderer,&r);
            }
            else if (MAP[y][x]=='2')
            {
                int idx=y*MAP_WIDTH+x;
                if (!eaten.count(idx))
                {
                    SDL_SetRenderDrawColor(renderer,255,255,255,255);
                    drawCircle(renderer, x*TILE_SIZE+TILE_SIZE/2,
                                         y*TILE_SIZE+TILE_SIZE/2,
                                         DOT_RADIUS);
                }
            }
        }

        // Pac-Man（有嘴巴動畫）
        SDL_SetRenderDrawColor(renderer,255,255,0,255);
        drawPacmanMouth(renderer, (int)pacPos.x,(int)pacPos.y,
                        PACMAN_RADIUS, pacDir, mouthAngle);

        // Ghosts（會走在通道中間）
        for (auto &g : ghosts)
        {
            SDL_SetRenderDrawColor(renderer, g.color.r,g.color.g,g.color.b,255);
            drawCircle(renderer, (int)g.pos.x,(int)g.pos.y,GHOST_RADIUS);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
