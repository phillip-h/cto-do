#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ncurses.h>

typedef struct tdlist {
    size_t len;
    size_t capacity;
    char **tasks;
    bool *done;
} tdlist;

int parse_args(int argc, char **argv);

char* get_dir(void);
char* get_path(void);
void check_dir(void);

void init_list(void);
void add_task(char *task, bool done, int pos);
void swap_task_up(int pos);
void remove_task(int pos);
void load_list(void);
void parse_task(char *str);
void write_list(void);
void free_list(void);

unsigned num_tasks(void);
unsigned num_tasks_done(void);
int adjust_pos(size_t pos);

void start_nc(void);
void end_nc(void);

void draw(void);
void draw_task(size_t n, char *task);
void draw_number(int n);
void draw_mark(int n);
void draw_title_bar(void);
void pad_to_end(char ch);
void vpad(int pad);
void draw_div(char *name);

void update_pads(void);

void new_task(void);
void edit_task(char* task);
int input(void);

/* configuration options */

const char *vn = "0.1";
const char *list_dir = ".local/share/cto-do";

const bool autosave = true;
const size_t task_buf_len = 2048 * sizeof(char);

const int E_OK      =  0;
const int E_BAD_ARG = -1;

const bool draw_title = true;

const bool highlight_whole_line = true;

const int top_pad = 1;
const int bottom_pad = 1;

const float edit_win_x = 0.5f;
const float edit_win_y = 0.1f;

const bool draw_numbers = true;
const bool zero_index = false;
const char *num_stop = ".";
const char *num_pad = "  ";

const bool draw_marks = true;
const char *mark = "[ ]";
const char *mark_done = "[x]";

const char div_magic_char = 0x07;
const bool draw_divs = true;
const bool draw_div_lines = true;
const char *div_left = " [";
const char *div_right = "]:";
const char div_pad = ' ';

const char *left_pad = "  ";
const char *task_pad = "  ";

const bool colors = true;

const int color_ind_off = 1;
const int color_ind_off_f = 1;
const int color_ind_off_b = -1;
const int color_ind_off_a = A_NORMAL;

const int color_ind_on= 2;
const int color_ind_on_f = 2;
const int color_ind_on_b = -1;
const int color_ind_on_a = A_NORMAL;

const int color_pos = 3;
const int color_pos_f = 6;
const int color_pos_b = -1;
const int color_pos_a = A_REVERSE;

const int color_box = 4;
const int color_box_f = 4;
const int color_box_b = -1;
const int color_box_a = A_REVERSE;

const int color_title = 5;
const int color_title_f = 4;
const int color_title_b = -1;
const int color_title_a = A_REVERSE;

/* default list name */
char *list_name = "to-do";

/* run-time globals */

tdlist list = {0, 0, NULL, NULL};
int num_width = 1;
unsigned offset = 0;
unsigned list_room = 0;
unsigned pos = 0;
bool new = false;

int parse_args(int argc, char **argv) {
    if (argc == 2) list_name = argv[1];
    return E_OK;
}

char* get_dir(void) {
    char *home = getenv("HOME");
    char *path = calloc(4096, sizeof(char));

    if (!path) {
        printf("Failed to allocate memory for dir path!\n");
        abort();
    }

    strcpy(path, home);
    strcat(path, "/");
    strcat(path, list_dir);

    return path;
}

char* get_path(void) {
    char *path = get_dir();
    strcat(path, "/");
    strcat(path, list_name);
    strcat(path, ".todo");

    return path;
}

void check_dir(void) {
    char *home = getenv("HOME");
    char *path = get_dir();
    struct stat st = {0};

    strcpy(path, home);
    strcat(path, "/");
    strcat(path, list_dir);

    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }

    free(path);
}

void init_list(void) {
    list.capacity = 1;
    char **tasks = malloc(list.capacity * sizeof(char*));
    bool *done = malloc(list.capacity * sizeof(bool));

    if (!tasks || !done) {
        printf("Failed to allocate initial memory for list!\n");
        abort();
    }

    list.tasks = tasks;
    list.done = done;
}

void add_task(char *task, bool done, int pos) {
    if (list.len >= list.capacity) {
        list.capacity *= 2;
        list.tasks = realloc(list.tasks, list.capacity * sizeof(char*));
        list.done = realloc(list.done, list.capacity * sizeof(bool));

        if (!list.tasks) {
            printf("Realloc() of %zu bytes failed!\n", list.capacity);
            abort();
        }
    }

    if (pos == -1 || pos == (int) list.len || list.len == 0) {
        list.tasks[list.len] = task;
        list.done[list.len] = done;
    } else {
        char **mem_dest = list.tasks + pos + 1;
        char **mem_src = list.tasks + pos;
        bool *done_dest = list.done + pos + 1;
        bool *done_src = list.done + pos;
        size_t mem_len = list.len - pos;
        memmove(mem_dest, mem_src, mem_len * sizeof(char*));
        memmove(done_dest, done_src, mem_len * sizeof(bool));
        list.tasks[pos] = task;
        list.done[pos] = done;
    }
    list.len += 1;
}

void swap_task_up(int pos) {
    if (pos == 0) {
        return;
    }

    char *tmp = list.tasks[pos - 1];
    bool done_tmp = list.done[pos - 1];

    list.tasks[pos - 1] = list.tasks[pos];
    list.done[pos - 1] = list.done[pos];

    list.tasks[pos] = tmp;
    list.done[pos] = done_tmp;
}

void remove_task(int pos) {
    char *task = list.tasks[pos];

    char **mem_dest = list.tasks + pos;
    char **mem_src = list.tasks + pos + 1;
    bool *done_dest = list.done + pos;
    bool *done_src = list.done + pos + 1;
    size_t mem_len = list.len - pos - 1;

    memmove(mem_dest, mem_src, mem_len * sizeof(char*));
    memmove(done_dest, done_src, mem_len * sizeof(bool));
    list.len -= 1;


    free(task);
}

void load_list(void) {
    char *path = get_path();
    FILE *f = NULL;
    char *str = NULL;
    char *new_task = NULL;

    f = fopen(path, "r");
    if (!f) {
        free(path);
        new = true;
        return;
    }

    str = malloc(task_buf_len);
    if (!str) {
        printf("Failed to allocate memory for file buffer!\n");
        goto end;
    }

    while (fgets(str, task_buf_len, f)) {
        new_task = malloc(task_buf_len);
        if (!new_task) {
            printf("Failed to allocate memory for new task!\n");
            goto end;
        }

        strcpy(new_task, str);
        parse_task(new_task);
    }

end:
    free(path);
    free(str);
    fclose(f);
}

void parse_task(char *str) {
    size_t len = strlen(str);
    if (str[len - 3] != ':') {
        printf("Warning -- malformed task:\n%s", str);
        printf("skipping it.\n\n");
        free(str);
        return;
    }

    str[len - 3] = '\0';
    add_task(str, str[len - 2] == '1' ? true : false, -1);
}

void write_list(void) {
    char *path = get_path();

    FILE *f = fopen(path, "w");
    if (!f) {
        perror("Error writing list");
        return;
    }

    for (size_t i = 0; i < list.len; ++i) {
        fputs(list.tasks[i], f);
        fputc(':', f);
        fputc(list.done[i] ? '1' : '0', f);
        fputc('\n', f);
    }

    fclose(f);
    free(path);
}

void free_list(void) {
    for (size_t i = 0; i < list.len; ++i)
        free(list.tasks[i]);
    free(list.tasks);
    free(list.done);
}

unsigned num_tasks(void) {
    unsigned count = 0;
    for (size_t i = 0; i < list.len; ++i)
        if (list.tasks[i][0] != div_magic_char)
            ++count;
    return count;
}

unsigned num_tasks_done(void) {
    unsigned count = 0;
    for (size_t i = 0; i < list.len; ++i)
        if (list.done[i] && list.tasks[i][0] != div_magic_char)
            ++count;
    return count;
}

int adjust_pos(size_t pos) {
    int count = 0;
    for (size_t i = 0; i < pos; ++i)
        if (list.tasks[i][0] != div_magic_char)
            ++count;
    return count;
}

void start_nc(void) {
    initscr();
    cbreak();
    curs_set(0);
    use_default_colors();
    if (colors && has_colors()) {
        start_color();
        init_pair(color_ind_off, color_ind_off_f, color_ind_off_b);
        init_pair(color_ind_on, color_ind_on_f, color_ind_on_b);
        init_pair(color_pos, color_pos_f, color_pos_b);
        init_pair(color_box, color_box_f, color_box_b);
        init_pair(color_title, color_title_f, color_title_b);
    }
    noecho();
    keypad(stdscr, TRUE);
}

void end_nc(void) {
    endwin();
    curs_set(1);
}

void draw(void) {
    size_t last_task = 0;

    update_pads();

    last_task = offset + list_room;
    if (last_task >= list.len) last_task = list.len;

    erase();

    draw_title_bar();
    vpad(top_pad);

    for (size_t i = offset; i < last_task; ++i) 
        draw_task(i, list.tasks[i]);

    refresh();
}

void draw_task(size_t n, char *task) {
    if (task[0] == div_magic_char) {
        if (n == pos) {
            attron(COLOR_PAIR(color_pos));
            attron(color_pos_a);
        }
        draw_div(task);
    } else {
        printw("%s", left_pad);
        draw_number(adjust_pos(n));

        if (n == pos) {
            attron(COLOR_PAIR(color_pos));
            attron(color_pos_a);
        }
        draw_mark(n);

        if (n == pos) {
            attron(COLOR_PAIR(color_pos));
            attron(color_pos_a);
        }
        printw("%s%s", task_pad, task);

    }

    if (task[0] != div_magic_char || !draw_div_lines) {
        if (n == pos) {
            pad_to_end(' ');
        } else {
            printw("\n");
        }
    }
        

    attroff(COLOR_PAIR(color_pos));
    attroff(color_pos_a);
}

void draw_number(int n) {
    if (!draw_numbers) return;
    printw("%*d", num_width, n + (zero_index? 0 : 1));
    printw("%s%s", num_stop, num_pad);
}

void draw_mark(int n) {
    if (!draw_marks) return;
    int color = list.done[n] ? COLOR_PAIR(color_ind_on) 
                             : COLOR_PAIR(color_ind_off);
    int attr = list.done[n] ? color_ind_on_a
                            : color_ind_off_a;

    attron(color);
    attron(attr);
    printw("%s", list.done[n] ? mark_done : mark);
    attroff(color);
    attroff(attr);
}

void draw_title_bar(void) {
    int num_total = num_tasks();
    int num_done = num_tasks_done();
    int percent_done = num_total ? 100 * num_done / list.len : 0;

    attron(COLOR_PAIR(color_title));
    attron(color_title_a);

    printw("%s ", list_name);
    if (new) printw("(new)");

    printw("-- %d tasks ", list.len);

    printw("%d/%d ", num_done, num_total);

    printw("-> %d%% ", percent_done);

    pad_to_end(' ');
    
    attroff(COLOR_PAIR(color_title));
    attroff(color_title_a);
}

void pad_to_end(char ch) {
    char buf[1024];
    int i = 0;
    for (i = 0; i < COLS - getcurx(stdscr); ++i)
        buf[i] = ch;
    buf[i] = '\0';
    printw("%s", buf);
}

void vpad(int pad) {
    for (int i = 0; i < pad; ++i)
        printw("\n");
}

void draw_div(char *name) {
    if (!draw_divs) return;

    printw("%s%s%s", div_left, name + 1, div_right);

    if (draw_div_lines) {
        pad_to_end(div_pad);
    }
}

void update_pads(void) {
    int len = list.len;
    num_width = 1;
    while (len > 1) {
        num_width += 1;
        len /= 10;
    }

    list_room = LINES;
    if (draw_title) list_room -= 1;
    list_room -= top_pad + bottom_pad;

    if (pos >= offset + list_room)
        offset += (pos - (offset + list_room)) + 1;

    if (pos < offset)
        offset = pos;
}

void new_task(void) {
    char *task = calloc(task_buf_len / sizeof(char), sizeof(char));
    if (!task) {
        printf("Failed to allocate memory for new task!\n");
        return;
    }

    edit_task(task);

    if (strlen(task) != 0) {
        add_task(task, false, list.len ? pos + 1 : pos);
        if (list.len != 0) {
            pos += 1;
        }
    } else {
        free(task);
    }
}

void edit_task(char *task) {
    int win_cols = (int) (COLS * edit_win_x);
    int win_rows = (int) (LINES * edit_win_y);
    int win_x = (int) (COLS - win_cols) / 2;
    int win_y = (int) (LINES - win_rows) / 4;
    WINDOW *input_win_box = newwin(win_rows + 2, win_cols + 2, win_y, win_x);
    WINDOW *input_win = newwin(win_rows, win_cols, win_y + 1, win_x + 1);

    int key = 0;
    int pos = strlen(task);
    int max_ind = task_buf_len / sizeof(char);
    bool loop = true;

    char *original = malloc(task_buf_len);
    if (!original) {
        printf("Failed to allocate memory for task backup!\n");
        return;
    }
    memcpy(original, task, task_buf_len);

    wattron(input_win_box, COLOR_PAIR(color_box));
    wattron(input_win_box, color_box_a);
    box(input_win_box, 0, 0);
    wprintw(input_win_box, "[edit task]");
    wrefresh(input_win_box);
    wattroff(input_win_box, COLOR_PAIR(color_box));
    wattroff(input_win_box, color_box_a);
    
    curs_set(1);

    while (loop) {
        werase(input_win);
        wprintw(input_win, "%s", task);
        wrefresh(input_win);

        key = getch();

        switch (key) {
            default:
                if (pos < max_ind) {
                    task[pos] = key;
                    pos += 1;
                }
                break;

            case KEY_BACKSPACE:
                if (pos != 0)
                    pos -= 1;
                task[pos] = '\0';
                break;
               
            case '\n':
               loop = false;
               break;

            case 27: // escape
               memcpy(task, original, task_buf_len);
               loop = false;
               break;

        }
    }

    curs_set(0);
    free(original);
}

int input(void) {
    int key = getch();
    switch (key) {
        case 'q':
            return false;

        case 'j':
        case KEY_DOWN:
            if (pos + 1 < list.len)
                pos += 1;
            break;

        case 'k':
        case KEY_UP:
            if (pos != 0)
                pos -= 1;
            break;

        case 'J':
            if (pos + 1 < list.len) {
                swap_task_up(pos + 1);
                pos += 1;
            }
            break;

        case 'K':
            if (pos != 0) {
                swap_task_up(pos);
                pos -= 1;
            }
            break;

       case 'R':
            if (list.len != 0) {
                remove_task(pos);
                if (pos >= list.len) {
                    pos -= 1;
                }
            }
            break;

       case 'E':
            if (list.len != 0)
                edit_task(list.tasks[pos]);
            break;

        case '\n':
            if (pos < list.len) 
                list.done[pos] ^= true;
            break;

       case ' ':
            new_task();
            break;
        
    }

    return true;
}

int main(int argc, char **argv) {
    int arg_rc = parse_args(argc, argv);
    if (arg_rc != E_OK) return arg_rc;

    check_dir();
    init_list();
    load_list();

    start_nc();
    do {
        draw();
        if (autosave) write_list();
    } while (input());

    end_nc();
    write_list();
    free_list();
}
