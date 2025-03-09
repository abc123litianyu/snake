#include <easyx/graphics.h>
#include <conio.h>
#include <time.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include<mmsystem.h>
using namespace std;

//▼▼▼ 预生成彩虹色数组 ▼▼▼
static COLORREF glowColors[360];
void initGlowColors() {
	for (int i = 0; i < 360; i++) {
		glowColors[i] = HSVtoRGB(i, 0.8f, 1.0f);
	}
}
//▼▼▼ 修改后的游戏状态枚举 ▼▼▼
enum GameState {
	START_SCREEN,
	IN_GAME,
	PAUSED,       // 新增暂停状态
	GAME_OVER
} gameState = START_SCREEN;
// 游戏参数设置
const int WIDTH = 80;
const int HEIGHT = 50;
const int BLOCK_SIZE = GetSystemMetrics(SM_CXSCREEN) / WIDTH / 1.5;
const int MAX_LENGTH = 300;
const int FOOD_COUNT = 5;
const int GAP = 0.4;
const int SNAKE_RADIUS = (BLOCK_SIZE - GAP) / 2;
int highScore = 0;
bool hasGoldenTriggered = false; // 新增彩蛋触发标志 

//▼▼▼ 动态速度控制参数 ▼▼▼
int baseDelay = 100;
int currentDelay = 100;
const int MIN_DELAY = 20;
const int MAX_DELAY = 200;

bool gameStarted = false;
DWORD gameStartTime = 0;
float currentPlayTime = 0.0f;
bool isGoldenEffect = false;
DWORD goldenStartTime = 0;

// 蛇结构体
struct Snake {
	int newDirection; // 新增成员 
	int x[MAX_LENGTH], y[MAX_LENGTH];
	int direction;
	int length;
	COLORREF headColor = RGB(0, 255, 0);
	COLORREF bodyColor = RGB(0, 200, 0);
} snake;

// 食物结构体
struct Food {
	int x, y;
	COLORREF color;
	bool active = true;
};
vector<Food> foods(FOOD_COUNT);
int score = 0;
bool gameover = false;

//▼▼▼ 智能食物生成算法 ▼▼▼
void createFood(int index) {
	do {
		foods[index].x = rand() % (WIDTH - 2) + 1;
		foods[index].y = rand() % (HEIGHT - 2) + 1;
		foods[index].color = RGB(rand() % 256, rand() % 256, rand() % 256);
	} while ([&]() {
	// 三维空间碰撞检测（xy坐标+时间维度）
	for (int i = 0; i < snake.length;  i++)
			if (abs(foods[index].x - snake.x[i]) < 2 &&
			    abs(foods[index].y - snake.y[i]) < 2)
				return true;
		for (int j = 0; j < FOOD_COUNT; j++)
			if (j != index && foods[j].active &&
			    foods[j].x == foods[index].x &&
			    foods[j].y == foods[index].y)
				return true;
		return false;
	}
	());
	foods[index].active = true;
}

void checkFood() {
	for (auto &f : foods) {
		if (!f.active)  continue;
		if (abs(snake.x[0] - f.x)*BLOCK_SIZE <= SNAKE_RADIUS &&
		    abs(snake.y[0] - f.y)*BLOCK_SIZE <= SNAKE_RADIUS) {
			f.active  = false;
			snake.length  = min(snake.length + 2,  MAX_LENGTH);
			score += 25;
			createFood(&f - &foods[0]);
		}
	}
}

//▼▼▼ 优化渲染函数 ▼▼▼
void drawCircle(int x, int y, int r, COLORREF color) {
	static POINT trails[5]; // 粒子拖尾缓存
	setfillcolor(color);
	solidcircle(x * BLOCK_SIZE + BLOCK_SIZE / 2,
	            y * BLOCK_SIZE + BLOCK_SIZE / 2, r);

	// 金蛋模式粒子效果
	if (isGoldenEffect && color != RGB(0, 80, 0)) {
		for (int i = 4; i > 0; i--) trails[i] = trails[i - 1];
		trails[0] = { x * BLOCK_SIZE, y * BLOCK_SIZE };
		for (int i = 0; i < 5; i++) {
			setfillcolor(RGB(255, 215, 0));
			solidcircle(trails[i].x, trails[i].y, r - i * 2);
		}
	}
}

COLORREF HSVtoRGB(int H, float S, float V) {
	float C = V * S;
	float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
	float m = V - C;

	float r, g, b;
	if (H >= 0 && H < 60) {
		r = C;
		g = X;
		b = 0;
	} else if (H >= 60 && H < 120) {
		r = X;
		g = C;
		b = 0;
	} else if (H >= 120 && H < 180) {
		r = 0;
		g = C;
		b = X;
	} else if (H >= 180 && H < 240) {
		r = 0;
		g = X;
		b = C;
	} else if (H >= 240 && H < 300) {
		r = X;
		g = 0;
		b = C;
	} else {
		r = C;
		g = 0;
		b = X;
	}

	return RGB((r + m) * 255, (g + m) * 255, (b + m) * 255);
}

//▼▼▼ 增强初始化函数 ▼▼▼
void initGame() {
	// 重置计时相关变量 
	gameStartTime = 0;        
	goldenStartTime = 0;
	currentPlayTime = 0.0f;
	hasGoldenTriggered = false;
	isGoldenEffect = false;
	gameState = START_SCREEN;
	hasGoldenTriggered = false;
	snake.x[0] = WIDTH / 2;
	snake.y[0] = HEIGHT / 2;
	snake.length  = 3;
	snake.direction   = -1;

	for (int i = 1; i < snake.length;  i++) {
		snake.x[i] = snake.x[i - 1] - 1;
		snake.y[i] = snake.y[i - 1];
	}

	srand((unsigned)time(NULL));
	for (int i = 0; i < FOOD_COUNT; i++)
		createFood(i);

	//▼▼▼ 高分存档保护机制 ▼▼▼
	FILE* fp = fopen("score.txt",   "r");
	if (fp) {
		fread(&highScore, sizeof(int), 1, fp);
		fclose(fp);
	}
	snake.direction  = -1;
	snake.newDirection  = -1; // 初始化新方向 	
}
void resetGame() {
	gameover = false;
	gameState = START_SCREEN;
	gameStarted = false;
	score = 0;
	initGame();
	SetWindowPos(GetHWnd(), HWND_TOP,
		(GetSystemMetrics(SM_CXSCREEN) - WIDTH * BLOCK_SIZE) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - HEIGHT * BLOCK_SIZE) / 2,
		0, 0, SWP_NOSIZE);
}
//▼▼▼ 优化绘制函数 ▼▼▼
void draw() {
	//▼▼▼ 在draw函数开始处增加 ▼▼▼
	if (gameState == PAUSED) return;  // 暂停时不更新游戏画面
	static int animValue = 0, flash = 0;
	animValue = (animValue + 5) % 360;
	flash = (flash + 1) % 30;

	// 双缓冲绘图初始化
	BeginBatchDraw();
	setbkcolor(RGB(30, 30, 30));
	cleardevice();

	// 蛇身呼吸灯渲染
	for (int i = snake.length - 1;  i >= 0; i--) {
		float breath = 0.1f * sin(GetTickCount() * 0.005f) + 0.9f;
		int baseGreen = 255 - static_cast<int>(i * 255 * 0.6f / snake.length  * breath);
		baseGreen = clamp(baseGreen, 50, 255);

		// 动态颜色计算
		COLORREF c;
		if (isGoldenEffect) {
			c = (i == 0) ? glowColors[animValue % 360] : RGB(255, 215 + animValue % 40, 0);
		} else {
			c = (i == 0) ? snake.headColor  : RGB(0, baseGreen, 0);
		}

		// 立体阴影渲染
		drawCircle(snake.x[i], snake.y[i], SNAKE_RADIUS + 1,
		           RGB(0, baseGreen / 2, 0));
		drawCircle(snake.x[i], snake.y[i], SNAKE_RADIUS, c);
	}

	// 食物光晕效果
	for (auto &f : foods) {
		if (!f.active)  continue;
		int r = SNAKE_RADIUS + (flash < 15 ? 2 : 0);
		drawCircle(f.x, f.y, r + 1, glowColors[(animValue + 60) % 360]);
		drawCircle(f.x, f.y, r, f.color);
	}

	//▼▼▼ 界面信息渲染 ▼▼▼
	TCHAR infoText[3][32];
	_stprintf_s(infoText[0], _T("★ 得分: %d ★"), score);
	_stprintf_s(infoText[1], _T("速度: %d%%"),
	            (MAX_DELAY - currentDelay) * 100 / (MAX_DELAY - MIN_DELAY));
	_stprintf_s(infoText[2], _T("时间: %.1fs"), currentPlayTime);

	// 动态文字特效
	settextstyle(24, 0, _T("Microsoft Yahei UI"));
	for (int i = 0; i < 3; i++) {
		settextcolor(RGB(0, 0, 0));
		outtextxy(WIDTH * BLOCK_SIZE - 220 + 2, 10 + 60 * i + 2, infoText[i]);
		settextcolor(glowColors[(animValue + i * 120) % 360]);
		outtextxy(WIDTH * BLOCK_SIZE - 220, 10 + 60 * i, infoText[i]);
	}

	FlushBatchDraw();
}

//▼▼▼ 优化游戏逻辑 ▼▼▼
void update() {
	if (!gameStarted) return;

	// 动态速度调节
	currentDelay = baseDelay - (score / 10);
	currentDelay = clamp(currentDelay, MIN_DELAY, MAX_DELAY);

	// 彩蛋模式检测
	if (!hasGoldenTriggered && gameStarted && currentPlayTime >= 20.25 && goldenStartTime == 0) {
		isGoldenEffect = true;
		goldenStartTime = GetTickCount();
		hasGoldenTriggered = true; // 标记已触发 
	}

	// 窗口震动效果
	if (isGoldenEffect) {
		int offset = (GetTickCount() % 100 < 50) ? 2 : -2;
		SetWindowPos(GetHWnd(), NULL, offset, offset, 0, 0, SWP_NOSIZE);
	}

	// 彩蛋结束处理
	if (isGoldenEffect && (GetTickCount() - goldenStartTime) >= 3000) {
		isGoldenEffect = false;
		goldenStartTime = 0;
		SetWindowPos(GetHWnd(), HWND_TOP,
		             (GetSystemMetrics(SM_CXSCREEN) - WIDTH * BLOCK_SIZE) / 2,
		             (GetSystemMetrics(SM_CYSCREEN) - HEIGHT * BLOCK_SIZE) / 2,
		             0, 0, SWP_NOSIZE);
	}

	// 移动蛇身
	for (int i = snake.length - 1; i > 0; i--) {
		snake.x[i] = snake.x[i - 1];
		snake.y[i] = snake.y[i - 1];
	}
	
// 移动头部
	switch (snake.direction) {
		case 0:
			snake.y[0]--;
			break; // 上
		case 1:
			snake.x[0]++;
			break; // 右
		case 2:
			snake.y[0]++;
			break; // 下
		case 3:
			snake.x[0]--;
			break; // 左
	}
	if (snake.newDirection  != -1) {
		if (abs(snake.direction  - snake.newDirection)  != 2) {
			snake.direction  = snake.newDirection; 
		}
		snake.newDirection  = -1; // 重置新方向 
	}
// 碰撞检测
	if (snake.x[0] <= 0 || snake.x[0] >= WIDTH - 1 ||
		snake.y[0] <= 0 || snake.y[0] >= HEIGHT - 1) {
		gameover = true;
		gameState = GAME_OVER; // 新增状态同步 
		return;
	}

// 自碰检测
	for (int i = 1; i < snake.length; i++) {
		if (snake.x[0] == snake.x[i] && snake.y[0] == snake.y[i]) {
			gameover = true;
			return;
		}
	}

// 吃食物检测
	checkFood();

}

//▼▼▼ 主循环优化 ▼▼▼
int main() {
	initgraph(WIDTH * BLOCK_SIZE, HEIGHT * BLOCK_SIZE + 20, EW_SHOWCONSOLE | EW_DBLCLKS);
	BeginBatchDraw();

	// 窗口样式优化
	LONG style = GetWindowLong(GetHWnd(), GWL_STYLE);
	style &= ~WS_THICKFRAME;
	SetWindowLong(GetHWnd(), GWL_STYLE, style);
	SetWindowPos(GetHWnd(), NULL, 0, 0, 0, 0,
	             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	initGlowColors();
	initGame();

	DWORD lastTick = GetTickCount();
	ExMessage msg;
	// 修改主循环结构
	while (true) {
		if (!gameover) {
			// 时间统计优化
			if (gameStarted) {
				if (gameStartTime == 0) { // 首次移动初始化时间
					gameStartTime = GetTickCount();
					goldenStartTime = 0; // 重置彩蛋计时
				}
				currentPlayTime = (GetTickCount() - gameStartTime) / 1000.0f;
			} else {
				currentPlayTime = 0.0f; // 未开始游戏时显示0
			}

			// 输入处理优化
			while (peekmessage(&msg, EX_KEY)) {
				if (gameState == GAME_OVER && msg.message  == WM_KEYDOWN) {
					gameover = false;
					gameState = START_SCREEN;
					initGame();
					break;
				}
				if (msg.message  == WM_KEYDOWN) {
					switch (gameState) {
						// 在 START_SCREEN 状态中添加方向键初始化 
					case START_SCREEN:
						if (msg.vkcode  == VK_SPACE) {
							gameState = IN_GAME;
							gameStarted = true;
							// 初始化方向为右（若用户未按方向键）
							if (snake.direction  == -1) snake.direction  = 1;
						}
						break;

						case IN_GAME:  // 游戏中
							switch (msg.vkcode)  {
								// 方向控制（仅响应合法转向）
								case 'W':
								case VK_UP:
									snake.newDirection  = 0; // 暂存新方向 
								break;
								case 'S':
								case VK_DOWN:
									snake.newDirection  = 2;
								break;
								case 'A':
								case VK_LEFT:
									snake.newDirection  = 3;
								break;
								case 'D':
								case VK_RIGHT:
									snake.newDirection  = 1;
								break;

								// 实时调速（限定范围）
								case 'R':
									currentDelay = max(currentDelay - 20, MIN_DELAY);
									break;
								case 'E':
									currentDelay = min(currentDelay + 20, MAX_DELAY);
									break;
								case 'P':
									gameState = PAUSED;
									gameStarted = false;
									break;
									//▼▼▼ 在按键处理switch结构中增加 ▼▼▼
									// 修改后的按键处理结构

							}
							break;
						case PAUSED:
						if (msg.vkcode  == VK_SPACE) {
							gameState = IN_GAME;
							gameStarted = true; // 保持游戏已启动状态 
							// 计算暂停时间偏移 
							DWORD pauseDuration = GetTickCount() - gameStartTime;
							gameStartTime += pauseDuration;
						}
						break;
						
						case GAME_OVER:  // 结束界面
							if (msg.vkcode  == 'R') {
								gameState = START_SCREEN;
								gameover = false;
								initGame();  // 重置所有游戏数据
							}
							break;
					}
				}
			}
			int elapsed = GetTickCount() - lastTick;
			Sleep(max(currentDelay - elapsed, 1));
			lastTick = GetTickCount();

			update();
			draw();
		} else if (gameState == PAUSED) {
			// 半透明遮罩层
			// 修改后的暂停界面渲染（第415行附近）
			setbkcolor(RGB(0, 0, 0));
			setfillcolor(RGB(0, 0, 0));
			solidrectangle(0, 0, WIDTH * BLOCK_SIZE, HEIGHT * BLOCK_SIZE);
			cleardevice();

			// 动态暂停文字
			static int pauseAnim = 0;
			pauseAnim = (pauseAnim + 2) % 360;
			settextstyle(40, 0, _T("微软雅黑"));
			settextcolor(HSVtoRGB(pauseAnim, 0.8, 1));
			outtextxy(WIDTH * BLOCK_SIZE / 2 - 80, HEIGHT * BLOCK_SIZE / 2 - 20, _T("游戏暂停"));

			// 恢复提示
			settextstyle(20, 0, _T("微软雅黑"));
			settextcolor(RGB(200, 200, 200));
			outtextxy(WIDTH * BLOCK_SIZE / 2 - 100, HEIGHT * BLOCK_SIZE / 2 + 40,
			          _T("按SPACE键继续游戏"));

			FlushBatchDraw();
		}else if (gameState == GAME_OVER) {// 新增死亡画面显示
			setbkcolor(RGB(30, 30, 30));
			cleardevice();

			// 绘制渐变背景
			for (int i = 0; i < HEIGHT; i++) {
				setlinecolor(HSVtoRGB(0, 0, i * 0.01f));
				line(0, i * BLOCK_SIZE, WIDTH * BLOCK_SIZE, i * BLOCK_SIZE);
			}

			// 显示结束信息
			settextstyle(40, 0, _T("Microsoft Yahei UI"));
			outtextxy(WIDTH * BLOCK_SIZE / 2 - 120, 100, _T("游戏结束!"));

			// 分数爆炸动画
			for (int i = 1; i <= 30; i++) {
				static int animFrame = 0;
				if (animFrame < 30) {
					BeginBatchDraw();
					cleardevice();
					settextcolor(HSVtoRGB(i * 12, 0.8, 1));
					TCHAR scoreText[50];
					// 显示历史最高记录（使用金色字体）
					settextcolor(RGB(255, 215, 0)); // 金色 
					settextstyle(28, 0, _T("微软雅黑"));
					TCHAR highScoreText[50];
					_stprintf_s(highScoreText, _T("历史最佳: %d"), highScore);
					outtextxy(WIDTH*BLOCK_SIZE/2 - 110, HEIGHT*BLOCK_SIZE/2 + 20, highScoreText);
					
					settextcolor(WHITE);
					_stprintf_s(scoreText, _T("最终得分: %d"), score);
					outtextxy(WIDTH*BLOCK_SIZE/2 - 100, HEIGHT*BLOCK_SIZE/2 - 40, scoreText);
					settextcolor(WHITE);
					settextstyle(24, 0, _T("微软雅黑"));
					outtextxy((WIDTH*BLOCK_SIZE-200)/2, HEIGHT*BLOCK_SIZE-100, 
						_T("按任意键重新开始")); // 这个输出语句需要保留 
					FlushBatchDraw();
					animFrame++;
				} else {
					animFrame = 0;
				}
				
			}
			if (score > highScore) {
				mciSendString(_T("play tada.wav"),  NULL, 0, NULL);
				settextcolor(RGB(255, 215, 0));
				highScore = score;
				FILE* fp = fopen("score.txt",  "w");
				if (fp) {
					fwrite(&highScore, sizeof(int), 1, fp);
					fclose(fp);
				}
			}
			
// 保持原有按键检测逻辑 
			ExMessage m;
			if (peekmessage(&m, EX_KEY) && m.message  == WM_KEYDOWN) {
				resetGame();
				gameover = false;
				gameState = START_SCREEN;
				gameStarted = false;
				initGame();
			}
			
			FlushBatchDraw();
		}
	}
	Sleep(2000);
	_getch();
	closegraph();
	return 0;
}
