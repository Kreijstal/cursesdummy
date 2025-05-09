#include <ncurses.h>
#include <string.h>

typedef void (*MenuCallback)(const char*);

void show_menu1(const char** options, int num_items, MenuCallback callback) {
    // Create selection menu
    WINDOW* menu = newwin(10, 30, 5, 5);
    box(menu, 0, 0);
    keypad(menu, TRUE);
    
    int choice = 0;
    
    while(1) {
        for(int i = 0; i < num_items; i++) {
            if(i == choice) wattron(menu, A_REVERSE);
            mvwprintw(menu, i+1, 2, "%s", options[i]);
            if(i == choice) wattroff(menu, A_REVERSE);
        }
        wrefresh(menu);
        
        int ch = wgetch(menu);
        switch(ch) {
            case KEY_UP: 
                choice = (choice > 0) ? choice-1 : num_items-1;
                break;
            case KEY_DOWN:
                choice = (choice < num_items-1) ? choice+1 : 0;
                break;
            case 10: // Enter key
                werase(menu);
                wrefresh(menu);
                delwin(menu);
                callback(options[choice]);
                return;
        }
    }
}

void show_menu2(const char** options, int num_items, MenuCallback callback) {
    // Save current cursor position
    int orig_y, orig_x;
    getyx(stdscr, orig_y, orig_x);
    
    // Clear screen and setup panes
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int menu_width = max_x / 3;
    
    // Create border between panes
    for(int y = 0; y < max_y; y++) {
        mvaddch(y, menu_width, ACS_VLINE);
    }
    
    int choice = 0;
    const char* option_contents[] = {
        "Option 1 Details:\n- Feature A\n- Feature B\n- Value: 42",
        "Option 2 Info:\n- Status: Active\n- Count: 17\n- Mode: Standard",
        "Option 3 Data:\n- Temperature: 23C\n- Pressure: 1013hPa\n- Humidity: 45%",
        "Exit the menu system"
    };
    
    while(1) {
        // Draw menu items
        for(int i = 0; i < num_items; i++) {
            if(i == choice) attron(A_REVERSE);
            mvprintw(i, 1, "%s", options[i]);
            if(i == choice) attroff(A_REVERSE);
        }
        
        // Draw content pane
        WINDOW* content = newwin(max_y-2, max_x-menu_width-2, 1, menu_width+1);
        wclear(content);
        box(content, 0, 0);
        mvwprintw(content, 1, 1, "%s", option_contents[choice]);
        wrefresh(content);
        delwin(content);
        
        refresh();
        
        int ch = getch();
        switch(ch) {
            case KEY_UP:
                choice = (choice > 0) ? choice-1 : num_items-1;
                break;
            case KEY_DOWN:
                choice = (choice < num_items-1) ? choice+1 : 0;
                break;
            case 10: // Enter key
                move(orig_y, orig_x);
                refresh();
                callback(options[choice]);
                return;
        }
    }
}

void menu_callback(const char* selected) {
    if(strcmp(selected, "Quit") != 0) {
        printw("Selected: %s\n", selected);
        refresh();
    }
}

int main() {
    // Initialize ncurses without clearing screen
    SCREEN* screen = newterm(NULL, stdout, stdin);
    set_term(screen);
    
    // Enable keypad and echo
    keypad(stdscr, TRUE);
    echo();
    
    // Enable scrolling
    scrollok(stdscr, TRUE);
    
    // REPL loop
    char input[256];
    const char* menu_items[] = {"Option 1", "Option 2", "Option 3", "Quit"};
    int num_menu_items = sizeof(menu_items)/sizeof(menu_items[0]);
    
    while(1) {
        printw("> ");
        getstr(input);
        printw("You entered: %s\n", input);
        
        if(strcmp(input, "exit") == 0) {
            break;
        }
        else if(strcmp(input, "select") == 0) {
            printw("Choose menu style (1 or 2): ");
            getstr(input);
            if(strcmp(input, "2") == 0) {
                show_menu2(menu_items, num_menu_items, menu_callback);
            } else {
                show_menu1(menu_items, num_menu_items, menu_callback);
            }
        }
    }
    
    // Clean up while preserving terminal state
    reset_shell_mode();
    delscreen(screen);
    return 0;
}