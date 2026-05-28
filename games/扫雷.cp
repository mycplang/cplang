// @名称 扫雷
// @描述 经典扫雷 | 10x10 15雷 左键插旗空格揭开

变量 屏宽 = 800;
变量 屏高 = 600;

变量 亮蓝 = {r:80,g:140,b:255,a:255};
变量 深蓝 = {r:15,g:15,b:35,a:255};
变量 白色 = {r:255,g:255,b:255,a:255};
变量 金色 = {r:255,g:215,b:0,a:255};
变量 灰色 = {r:150,g:150,b:160,a:255};
变量 红色 = {r:255,g:60,b:60,a:255};
变量 黑色 = {r:0,g:0,b:0,a:255};
变量 绿色 = {r:80,g:220,b:80,a:255};
变量 酸橙色 = {r:0,g:255,b:0,a:255};

函数 扫雷初始化(宽, 高, 雷数) {
    变量 g = 表();
    表设(g, "宽", 宽);
    表设(g, "高", 高);
    表设(g, "雷数", 雷数);
    
    // 创建格子数组 (0=空, -1=雷, >0=数字)
    变量 total = 宽 * 高;
    变量 grid = [];
    变量 i = 0;
    当 (i < total) { push(grid, 0); i = i + 1; }
    
    // 放雷
    变量 m = 0;
    当 (m < 雷数) {
        变量 r = 随机整数(0, total - 1);
        如果 (grid[r] == 0) { grid[r] = -1; m = m + 1; }
    }
    
    // 计算数字
    i = 0;
    当 (i < total) {
        如果 (grid[i] != -1) {
            变量 count = 0;
            变量 y = 整除(i, 宽);
            变量 x = i - y * 宽;
            变量 dy = -1;
            当 (dy <= 1) {
                变量 dx = -1;
                当 (dx <= 1) {
                    变量 nx = x + dx;
                    变量 ny = y + dy;
                    如果 (nx >= 0 && nx < 宽 && ny >= 0 && ny < 高) {
                        如果 (grid[ny * 宽 + nx] == -1) { count = count + 1; }
                    }
                    dx = dx + 1;
                }
                dy = dy + 1;
            }
            grid[i] = count;
        }
        i = i + 1;
    }
    
    表设(g, "grid", grid);
    
    // 揭开状态
    变量 revealed = [];
    i = 0;
    当 (i < total) { push(revealed, 0); i = i + 1; }
    表设(g, "revealed", revealed);
    
    // 旗帜
    变量 flags = [];
    i = 0;
    当 (i < total) { push(flags, 0); i = i + 1; }
    表设(g, "flags", flags);
    
    表设(g, "gameOver", 0);
    表设(g, "won", 0);
    
    返回 g;
}

函数 扫雷揭开(g, x, y) {
    变量 宽 = 表取(g, "宽");
    变量 高 = 表取(g, "高");
    变量 idx = y * 宽 + x;
    变量 revealed = 表取(g, "revealed");
    变量 flags = 表取(g, "flags");
    
    如果 (flags[idx] == 1) { 返回; }
    如果 (revealed[idx] == 1) { 返回; }
    
    变量 grid = 表取(g, "grid");
    revealed[idx] = 1;
    
    如果 (grid[idx] == -1) {
        表设(g, "gameOver", 1);
        返回;
    }
    
    如果 (grid[idx] == 0) {
        // 递归展开空白区域
        变量 dy = -1;
        当 (dy <= 1) {
            变量 dx = -1;
            当 (dx <= 1) {
                如果 (dx != 0 || dy != 0) {
                    变量 nx = x + dx;
                    变量 ny = y + dy;
                    如果 (nx >= 0 && nx < 宽 && ny >= 0 && ny < 高) {
                        扫雷揭开(g, nx, ny);
                    }
                }
                dx = dx + 1;
            }
            dy = dy + 1;
        }
    }
}

函数 扫雷绘制(g, px, py, 格宽) {
    变量 宽 = 表取(g, "宽");
    变量 高 = 表取(g, "高");
    变量 grid = 表取(g, "grid");
    变量 revealed = 表取(g, "revealed");
    变量 flags = 表取(g, "flags");
    变量 gameOver = 表取(g, "gameOver");
    
    变量 y = 0;
    当 (y < 高) {
        变量 x = 0;
        当 (x < 宽) {
            变量 idx = y * 宽 + x;
            变量 rx = px + x * 格宽;
            变量 ry = py + y * 格宽;
            
            如果 (revealed[idx] == 1) {
                如果 (grid[idx] == -1) {
                    drawRectangle(rx, ry, 格宽 - 1, 格宽 - 1, 红色);
                } 否则 {
                    drawRectangle(rx, ry, 格宽 - 1, 格宽 - 1, 白色);
                    如果 (grid[idx] > 0) {
                        绘制文本(toString(grid[idx]), rx + 格宽/2 - 6, ry + 格宽/2 - 10, 格宽 - 6, 黑色);
                    }
                }
            } 否则 {
                变量 色 = {r:180, g:180, b:200, a:255};
                如果 (flags[idx] == 1) { 色 = {r:255, g:180, b:60, a:255}; }
                如果 (gameOver && grid[idx] == -1) { 色 = {r:255, g:100, b:100, a:255}; }
                drawRectangle(rx, ry, 格宽 - 1, 格宽 - 1, 色);
            }
            x = x + 1;
        }
        y = y + 1;
    }
}

函数 扫雷计分(g) {
    变量 宽 = 表取(g, "宽");
    变量 高 = 表取(g, "高");
    变量 revealed = 表取(g, "revealed");
    变量 total = 宽 * 高;
    变量 count = 0;
    变量 i = 0;
    当 (i < total) {
        如果 (revealed[i] == 1) { count = count + 1; }
        i = i + 1;
    }
    返回 count * 10;
}

函数 扫雷() {
    变量 宽 = 10;
    变量 高 = 10;
    变量 雷 = 15;
    变量 格 = 40;
    
    变量 棋盘W = 宽 * 格;
    变量 棋盘H = 高 * 格;
    变量 px = (屏宽 - 棋盘W) / 2;
    变量 py = (屏高 - 棋盘H) / 2;
    
    变量 g = 扫雷初始化(宽,宽, 高, 雷);
    变量 分数 = 0;
    变量 结束 = 0;
    
    当 (窗口应关闭() == 假) {
        如果 (键盘刚按下(键_退出)) { 返回 分数; }
        如果 (结束 && 键盘刚按下(键_R)) {
            g = 扫雷初始化(宽,宽, 高, 雷);
            分数 = 0; 结束 = 0;
        }
        
        如果 (结束 == 0 && 鼠标刚按下()) {
            变量 mx = 鼠标X();
            变量 my = 鼠标Y();
            如果 (mx >= px && mx < px + 棋盘W && my >= py && my < py + 棋盘H) {
                变量 gx = 整除((mx - px), 格);
                变量 gy = 整除((my - py), 格);
                变量 idx = gy * 宽 + gx;
                变量 flags = 表取(g, "flags");
                变量 revealed = 表取(g, "revealed");
                如果 (revealed[idx] == 0) {
                    flags[idx] = 1 - flags[idx];
                }
            }
        } 否则 {
            如果 (结束 == 0 && 键盘刚按下(键_空格)) {
                变量 mx = 鼠标X();
                变量 my = 鼠标Y();
                如果 (mx >= px && mx < px + 棋盘W && my >= py && my < py + 棋盘H) {
                    变量 gx = 整除((mx - px), 格);
                    变量 gy = 整除((my - py), 格);
                    扫雷揭开(g, gx, gy);
                    分数 = 扫雷计分(g);
                    如果 (表取(g, "gameOver") == 1) { 结束 = 1; }
                }
            }
        }
        
        变量 gameOver = 表取(g, "gameOver");
        如果 (gameOver == 1) { 结束 = 1; }
        
        开始绘制();
        清除背景(深蓝);
        绘制文本("扫雷  分数:" + toString(分数), px, py - 30, 24, 金色);
        绘制文本("左键插旗 空格揭开", px + 200, py - 26, 16, 灰色);
        扫雷绘制(g, px, py, 格);
        如果 (结束) {
            绘制文本("游戏结束!", px + 棋盘W/2 - 60, py + 棋盘H/2 - 10, 28, 红色);
            绘制文本("R重来 ESC返回", px + 棋盘W/2 - 80, py + 棋盘H/2 + 22, 16, 灰色);
        }
        结束绘制();
    }
    返回 分数;
}

// INSERT_GAMES_HERE

初始化窗口(800, 600, "扫雷");
设置目标帧率(60);

扫雷();

关闭窗口();
