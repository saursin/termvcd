#include <string>
#include <ncurses.h>
class Window
{
public:
    std::string title;
    bool is_selected=false;
    WINDOW * win;
    int height; int width; int y; int x;


    Window(int height, int width, int y, int x, std::string title=""): height(height), width(width), y(y), x(x), title(title)
    { 
        win = newwin(this->height, this->width, this->y, this->x); box(win, 0, 0);
        print_win_outline();
        wrefresh(win);
    }

    virtual void print_win_outline()
    {
        box(win, 0, 0);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        this->move(0, 2);

        wattron(win, A_BOLD);
        if (this->is_selected) wattron(win, COLOR_PAIR(1));
        wprintw(win, " %s ", this->title.c_str());
        if (this->is_selected) wattroff(win, COLOR_PAIR(1));
        wattroff(win, A_BOLD);
    }

    virtual void move(int y, int x)
    {
        wmove(win, y, x);
    }

    virtual void refresh()
    {
        wclear(win);
        print_win_outline();
        print();
        wrefresh(win);
    }

    virtual void keypress(char key) = 0;
    
    virtual void print() = 0;
};

