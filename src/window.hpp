#include <string>
#include <ncurses.h>
class Window
{
public:
    std::string title;
    WINDOW * win;
    int height; int width; int y; int x;


    Window(int height, int width, int y, int x, std::string title=""): height(height), width(width), y(y), x(x), title(title)
    { 
        win = newwin(this->height, this->width, this->y, this->x); box(win, 0, 0);
        _printheader();
    }

    virtual void _printheader(bool selected=false)
    {
        this->move(0, 2);
        if (selected)
        {
            init_pair(1, COLOR_RED, COLOR_BLACK);

            wattron(win, COLOR_PAIR(1));
            wattron(win, A_BOLD);
            wprintw(win, " %s ", this->title.c_str());
            wattroff(win, A_BOLD);      
            wattroff(win, COLOR_PAIR(1));
        }
        else
            wprintw(win, " %s ", this->title.c_str());
    }

    virtual void move(int y, int x)
    {
        wmove(win, y, x);
    }

    virtual void clear()
    {
        wclear(win);
        _printheader();
    }

    virtual void refresh()
    {
        wrefresh(win);
    }

    virtual void keypress(char key) = 0;
    
    virtual void populate() = 0;
};

