#include <easyx/graphics.h>
#include <conio.h>
#include <time.h>
#include <string>
#include <vector>
#include <cmath>
using namespace std;

//▼▼▼ 新增预生成颜色数组 ▼▼▼ 
static COLORREF glowColors[360];
void initGlowColors() {
	for(int i = 0; i < 360; i++) {
		glowColors[i] = HSVtoRGB(i, 0.8f, 1.0f);
	}
}
// 游戏参数设置

const int WIDTH = 80; // 横向格子数
const int HEIGHT = 50; // 纵向格子数
const int BLOCK_SIZE = GetSystemMetrics(SM_CXSCREEN) / WIDTH / 1.5; // 单位格子尺寸
const int MAX_LENGTH = 300; // 蛇的最大长度
const int FOOD_COUNT = 5; // 同时存在的食物数量
const int GAP = 0.4; // 蛇身间隙像素
const int SNAKE_RADIUS = (BLOCK_SIZE - GAP)/2; // 蛇身半径
int highScore = 0;
int baseDelay = 100;   // 基础移动间隔（毫秒）
int currentDelay = 100; // 当前实际延迟 
const int MIN_DELAY = 20;  // 最快速度限制 
const int MAX_DELAY = 200; // 最慢速度限制 
bool gameStarted = false;  // 游戏启动标志 
DWORD gameStartTime = 0;       // 游戏开始时间戳 
float currentPlayTime = 0.0f;  // 当前游玩时间（秒）
bool isGoldenEffect = false;  // 彩蛋触发标志 
// 蛇结构体
struct Snake {
	int x[MAX_LENGTH], y[MAX_LENGTH]; // 身体坐标
	int direction; // 移动方向：0上 1右 2下 3左
	int length; // 当前长度
	COLORREF headColor = RGB(0,255,0); // 头部颜色
	COLORREF bodyColor = RGB(0,200,0); // 身体颜色
} snake;

// 食物结构体
struct Food {
	int x, y;
	COLORREF color;
	bool active = true;
};
vector<Food> foods(FOOD_COUNT);
int score = 0; // 得分统计
bool gameover = false; // 游戏状态

// 生成新食物
void createFood(int index) {
	do {
		foods[index].x = rand() % (WIDTH-2) +1;
		foods[index].y = rand() % (HEIGHT-2)+1;
		foods[index].color = RGB(rand()%256, rand()%256, rand()%256);
	} while([&](){
		for(int i=0; i<snake.length; i++)
			if(abs(foods[index].x - snake.x[i]) < 2 &&
				abs(foods[index].y - snake.y[i]) < 2)
				return true;
		for(int j=0; j<FOOD_COUNT; j++)
			if(j != index && foods[j].active &&
				foods[j].x == foods[index].x &&
				foods[j].y == foods[index].y)
				return true;
		return false;
	}());
	foods[index].active = true;
}
void checkFood() {
	for(auto &f : foods) {
		if(!f.active) continue;
		if(abs(snake.x[0]-f.x)*BLOCK_SIZE <= SNAKE_RADIUS &&
			abs(snake.y[0]-f.y)*BLOCK_SIZE <= SNAKE_RADIUS) {
			f.active = false;
			snake.length = min(snake.length+2, MAX_LENGTH);
			score += 25;
			createFood(&f - &foods[0]);
		}
	}
}
void drawCircle(int x, int y, int r, COLORREF color) {
	setfillcolor(color);
	solidcircle(x*BLOCK_SIZE + BLOCK_SIZE/2,
		y*BLOCK_SIZE + BLOCK_SIZE/2, r);
}
// HSV转RGB颜色空间转换函数 
COLORREF HSVtoRGB(int H, float S, float V) {
	float C = V * S;
	float X = C * (1 - abs(fmod(H/60.0, 2) - 1));
	float m = V - C;
	
	float r, g, b;
	if(H >= 0 && H < 60) { r = C; g = X; b = 0; }
	else if(H >= 60 && H < 120) { r = X; g = C; b = 0; }
	else if(H >= 120 && H < 180) { r = 0; g = C; b = X; }
	else if(H >= 180 && H < 240) { r = 0; g = X; b = C; }
	else if(H >= 240 && H < 300) { r = X; g = 0; b = C; }
	else { r = C; g = 0; b = X; }
	
	return RGB((r+m)*255, (g+m)*255, (b+m)*255);
}
// 游戏初始化
void initGame() {
	
	snake.x[0] = WIDTH/2;
	snake.y[0] = HEIGHT/2;
	snake.length = 3;
	snake.direction  = -1;
	
	for(int i=1; i<snake.length; i++) {
		snake.x[i] = snake.x[i-1]-1;
		snake.y[i] = snake.y[i-1];
	}
	srand((unsigned)time(NULL));
	for(int i=0; i<FOOD_COUNT; i++)
		createFood(i);
	FILE* fp = fopen("score.txt",  "r");
	if(fp) {
		fread(&highScore, sizeof(int), 1, fp);
		fclose(fp);
	}
}

// 绘制游戏界面
// 绘制游戏界面（集成实时分数显示）
//▼▼▼ 优化后的绘制函数 ▼▼▼ 
void draw() {
	static int animValue = 0, flash = 0;
	animValue = (animValue + 5) % 360;
	flash = (flash + 1) % 30;
	
	// 使用预生成颜色提升性能 
	COLORREF speedColor = glowColors[(animValue + 200) % 360]; // 固定蓝色系偏移 
	
	// 清屏与背景初始化 
	setbkcolor(RGB(30,30,30));
	cleardevice();
	
	// 蛇身渲染（优化颜色计算）
	for(int i = snake.length-1;   i >= 0; i--) {
		const float depth = 0.6f;
		const float breath = 0.1f * sin(GetTickCount() * 0.005f) + 0.9f;
		int baseGreen = 255 - static_cast<int>(i * 255 * depth / snake.length   * breath);
		baseGreen = (baseGreen < 50) ? 50 : (baseGreen > 255) ? 255 : baseGreen;
		
		// 使用预生成阴影颜色 
		drawCircle(snake.x[i], snake.y[i], SNAKE_RADIUS+1,
			(i == 0) ? RGB(0, 80, 0) : RGB(0, baseGreen/2, 0));
		
		COLORREF c;
		if(isGoldenEffect) {
			if(i == 0) {
				c = HSVtoRGB(animValue%360, 0.9, 1.0); // 头部彩虹渐变 
			} else {
				c = RGB(255,215 + animValue%40, 0);    // 身体金色脉动 
			}
		} else {
			c = (i == 0) ? snake.headColor  : RGB(0, baseGreen, 0);
		}
		drawCircle(snake.x[i], snake.y[i], SNAKE_RADIUS, c);
	}
	
	// 优化后的食物渲染 
	for(auto &f : foods) {
		if(!f.active)  continue;
		int r = SNAKE_RADIUS + (flash<15 ? 2 : 0);
		drawCircle(f.x, f.y, r+1, glowColors[(animValue + 60) % 360]); // 使用预生成光晕色 
		drawCircle(f.x, f.y, r, f.color);  
	}
	
	//▼▼▼ 优化文字渲染层 ▼▼▼ 
	TCHAR scoreText[32], speedText[32];
	_stprintf_s(scoreText, _T("★ 目前分数: %d ★"), score);
	
	// 分数显示（优化阴影效果）
	settextstyle(24, 0, _T("Microsoft Yahei UI"));
	settextcolor(RGB(0,0,0));
	outtextxy(WIDTH*BLOCK_SIZE - 220 + 2, 10 + 2, scoreText);
	settextcolor(RGB(255,215,0) | (glowColors[animValue % 360] & 0x00FFFFFF)); // 混合渐变 
	outtextxy(WIDTH*BLOCK_SIZE - 220, 10, scoreText);
	
	// 速度显示（优化动态计算）
	static float smoothSpeed = 0.0f;
	int realSpeed = (MAX_DELAY - currentDelay) * 100 / (MAX_DELAY - MIN_DELAY);
	smoothSpeed += (realSpeed - smoothSpeed) * 0.1f;
	_stprintf_s(speedText, _T("速度: %d%%"), static_cast<int>(smoothSpeed));
	
	settextstyle(20, 0, _T("Microsoft Yahei UI"));
	settextcolor(RGB(0,0,0));
	outtextxy(WIDTH*BLOCK_SIZE - 220 + 2, 40 + 2, speedText);
	settextcolor(speedColor);
	outtextxy(WIDTH*BLOCK_SIZE - 220, 40, speedText);
	
	
	
	TCHAR timeText[32];
	_stprintf_s(timeText, _T("时间: %.1fs"), currentPlayTime);
	
	settextstyle(20, 0, _T("Microsoft Yahei UI"));
	settextcolor(RGB(0,0,0));
	outtextxy(WIDTH*BLOCK_SIZE - 220 + 2, 70 + 2, timeText);
	settextcolor(glowColors[(animValue + 120) % 360]); // 使用彩虹色系 
	outtextxy(WIDTH*BLOCK_SIZE - 220, 70, timeText);
}
//▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲ 
// 游戏逻辑更新
void update() {
	if(!gameStarted) return;
// 移动蛇身
	for(int i=snake.length-1; i>0; i--) {
		snake.x[i] = snake.x[i-1];
		snake.y[i] = snake.y[i-1];
	}
	
// 移动头部
	switch(snake.direction) {
		case 0: snake.y[0]--; break; // 上
		case 1: snake.x[0]++; break; // 右
		case 2: snake.y[0]++; break; // 下
		case 3: snake.x[0]--; break; // 左
	}
	
// 碰撞检测
	if(snake.x[0]<=0 || snake.x[0]>=WIDTH-1 ||
		snake.y[0]<=0 || snake.y[0]>=HEIGHT-1) {
		gameover = true;
		return;
	}
	
// 自碰检测
	for(int i=1; i<snake.length; i++) {
		if(snake.x[0]==snake.x[i] && snake.y[0]==snake.y[i]) {
			gameover = true;
			return;
		}
	}
	
// 吃食物检测
	checkFood();
	if(fabs(fmod(currentPlayTime, 20.25)) <= 0.3) {
		isGoldenEffect = true;
		// 震动效果（窗口偏移3像素）
		static bool shake = false;
		SetWindowPos(GetHWnd(), NULL, shake?3:0, shake?0:3, 0, 0, SWP_NOSIZE);
		shake = !shake;
	} else {
		isGoldenEffect = false;
	}
}

int main() {
	//▼▼▼ 双缓冲初始化 ▼▼▼ 
	initgraph(WIDTH*BLOCK_SIZE, HEIGHT*BLOCK_SIZE+20, EW_SHOWCONSOLE | EW_DBLCLKS);
	BeginBatchDraw();
	
	// 窗口样式调整 
	LONG style = GetWindowLong(GetHWnd(), GWL_STYLE);
	style &= ~WS_THICKFRAME;
	SetWindowLong(GetHWnd(), GWL_STYLE, style);
	SetWindowPos(GetHWnd(), NULL, 0, 0, 0, 0, 
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	
	initGlowColors(); // 初始化颜色梯度 
	initGame();
	
	//▼▼▼ 优化主循环结构 ▼▼▼ 
	DWORD lastTick = GetTickCount();
	ExMessage msg;
	while(!gameover) {
		if(gameStarted && gameStartTime == 0) {
			gameStartTime = GetTickCount(); // 记录游戏开始时间 
		}
		if(gameStarted) {
			currentPlayTime = (GetTickCount() - gameStartTime) / 1000.0f;
		}
		DWORD currentTick = GetTickCount();
		float deltaTime = (currentTick - lastTick)/1000.0f;
		(void)deltaTime;  // 显式忽略警告 
		lastTick = currentTick;
		while (peekmessage(&msg, EX_KEY)) {
			if (msg.message == WM_KEYDOWN) {
				switch (msg.vkcode) {
					case 'W': case VK_UP: // 支持方向键
					if(!gameStarted) gameStarted = true;
					if(snake.direction != 2) snake.direction = 0;
					break;
				case 'S': case VK_DOWN:
					if(!gameStarted) gameStarted = true;
					if(snake.direction != 0) snake.direction = 2;
					break;
				case 'A': case VK_LEFT:
					if(!gameStarted) gameStarted = true;
					if(snake.direction != 1) snake.direction = 3;
					break;
				case 'D': case VK_RIGHT:
					if(!gameStarted) gameStarted = true;
					if(snake.direction != 3) snake.direction = 1;
					break;
					case 'R': // 加速 
					currentDelay = max(currentDelay - 20, MIN_DELAY);
					break;
					case 'E': // 减速 
					currentDelay = min(currentDelay + 20, MAX_DELAY);
					break;
				}
			}
			cleardevice();
			update();
			draw();
			FlushBatchDraw();
			
			// 动态帧率控制 
			int elapsed = GetTickCount() - currentTick;
			Sleep(max(currentDelay - elapsed, 1));
		}
		EndBatchDraw();
		update();
		draw();
		Sleep(currentDelay);
		
		static int foodTimer = 0;
		if(++foodTimer % 200 == 0) {
			for(auto &f : foods) {
				if(!f.active) {
					createFood(&f - &foods[0]);
					break;
				}
			}
		}
	} // 正确结束while循环
	
// 游戏结束界面
	settextcolor(RED);
	settextstyle(40, 0, _T("Microsoft Yahei UI"));
	outtextxy(80, 100, _T("游戏结束!"));
	outtextxy(80, 150, ("最终得分: " + to_string(score)).c_str());
	outtextxy(80, 200, ("最高记录: " + to_string(highScore)).c_str());
	Sleep(2000);
	_getch();
	if(score > highScore) {
		highScore = score;
		FILE* fp = fopen("score.txt",  "w");
		if(fp) {
			fwrite(&highScore, sizeof(int), 1, fp);
			fclose(fp);
		}
	}
	closegraph();
	return 0;
}
