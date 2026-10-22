#pragma once
constexpr int WHITE = 1, BLACK = 0, TEXT_ALIGN_CENTER = 1, TEXT_ALIGN_LEFT = 0;
constexpr int ArialMT_Plain_24 = 24, ArialMT_Plain_10 = 10;
struct SSD1306Wire {
    SSD1306Wire(int, int, int) {}
    void init() {}
    void flipScreenVertically() {}
    void clear() {}
    void setColor(int) {}
    void setTextAlignment(int) {}
    void fillRect(int,int,int,int) {}
    void setFont(int) {}
    void drawString(int,int,const char*) {}
    void display() {}
};
