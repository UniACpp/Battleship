/* ---------------------------------*- C -*--------------------------------- *\
 *
 * Name
 *
 * Description
 *
 * Author
 *
\* ------------------------------------------------------------------------- */

/* #include <assert.h> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_WIDTH 99
#define MAX_HEIGHT MAX_WIDTH

#define SUCCESS 0
#define INPUT_ERROR -1
#define BUFFER_ERROR -2

#define TRUE 1
#define FALSE 0

/* number of ships (value) of each length (postion in array) */
#define NUM_SHIPS_INIT {0, 0, 4, 3, 2, 1}
#define MIN_SHIP_LENGTH 2
#define MAX_SHIP_LENGTH 5

/* The gap between left and right fields */
#define print_gap() (printf("        "))


static const char *const rowFormats[] =
{
        "   |",     /* 0: unhit - water */
        "~~~|",     /* 1: hit water */
        "###|",     /* 2: unhit ship on the right field*/
        "...|",     /* 3: hit ship */
        "XXX|"      /* 4: sunken ship */
};

enum shipstate
{
        NONE = 0,   /* 0: unhit - water */
        SPLASH,     /* 1: hit water */
        UNHIT,      /* 2: unhit ship on the right field*/
        HIT,        /* 3: hit ship */
        SUNK        /* 4: sunken ship */
};

typedef enum shipstate shipstate_t;

typedef struct {
        int nx;
        int ny;
        /* num_of_ship_left[i] is the number of i long ships left */
        int nShipsRemaining_left[MAX_SHIP_LENGTH + 1];
        int nShipsRemaining_right[MAX_SHIP_LENGTH + 1];
        /* 0 = unhit water, 1 = hit water, 2 = unhit ship, 3 = hit ship, 4 = sunken ship*/
        shipstate_t **data_left;
        shipstate_t **data_right;
} play_fields_t;


/* allocates one 2D-array for the battleship field */
static shipstate_t ** alloc_field(int nx, int ny)
{
        /* Room for all rows (Array of pointers) */
        shipstate_t **array2d = (shipstate_t**) calloc(ny, sizeof(shipstate_t*));

        /* All data as a single slab */
        shipstate_t *slabs = (shipstate_t*) calloc(nx * ny, sizeof(shipstate_t));

        int i;
        for (i = 0; i < ny; ++i) {
                array2d[i] = slabs;
                slabs += nx;
        }

        return array2d;
}

/* frees the memory used in both the data_left and data_right 2D-array */
static void free_field(play_fields_t *fld)
{
        free(fld->data_left[0]);
        free(fld->data_right[0]);
        free(fld->data_left);
        free(fld->data_right);
}

/* prints horizontal line, deviding the rows (--+---+---+...) */
void print_hline(int nx)
{
        int coli;

        printf("--+");
        for (coli = 0; coli < nx; ++coli) {
                printf("---+");
        }
}

/* prints top row (  | A | B | C | ... ) */
void print_top_row(int nx)
{
        int coli;

        printf("  |");
        /* format as "AA" etc if there are more than 26 cols */
        for (coli = 0; coli < nx; ++coli) {
                printf("%c%c |", (coli < 26 ? ' ' : (coli / 26 - 1) + 'A'), ((coli % 26) + 'A'));
        }
}

/* prints row including the numbering at the beginning of the line ( 1|~~~|XXX|~~~|...) */
void print_row(const shipstate_t row[], int nx, int row_num, int is_left_field)
{
        int i;

        printf("%2i|", row_num);

        for (i = 0; i < nx; ++i) {
                int state = row[i];
                if (is_left_field && state == UNHIT) {
                       /*state = NONE; *//* keep hidden */
                }

                printf(rowFormats[state]);
        }
}

/* prints both entire battleship-fields */
void print_field(play_fields_t *fld)
{
        /* copies of the struct values */
        const int nx = fld->nx;
        const int ny = fld->ny;
        shipstate_t **data_left = fld->data_left;
        shipstate_t **data_right = fld->data_right;

        int row;

        /* top row:   | A | B |...          | A | B |... */
        print_top_row(nx);
        print_gap();
        print_top_row(nx);
        printf("\n");
        for (row = 0; row < ny; ++row) {
                /* horizontal line: --+---+---+...        --+---+---+... */
                print_hline(nx);
                print_gap();
                print_hline(nx);
                printf("\n");
                /* other rows:   <row>|~~~|XXX|...          |   |XXX|... */
                print_row(data_left[row], nx, row + 1, 0);
                print_gap();
                print_row(data_right[row], nx, row + 1, 0);
                printf("\n");
        }
        /* horizontal line: --+---+---+...        --+---+---+... */
        print_hline(nx);
        print_gap();
        print_hline(nx);
        printf("\n");
}

int flush_buff()
{
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        return (c == EOF) ? BUFFER_ERROR : SUCCESS;
}

/* e.g.: stdin: "11e" --> *x = 4, *y = 10
 * production:
 * <coordinate>  --> <digit>[<digit>]<letter>[letter]
 * <coordinate>  --> <letter>[letter]<digit>[<digit>]
 * <digit> --> '0'|'1'|...|'9'
 * <letter> --> 'A'|'B'|...|'Z'|'a'|'b'|...|'z'
 */
int scan_coordinate(int *x, int *y, int nx, int ny)
{
        /* mode: tracks which stage of parsing the function is in:
         * 0 : numbers first: 1st digit
         * 1 : numbers first: 2nd digit
         * 2 : numbers first: 1st letter
         * 3 : numbers first: 2nd letter
         * 4 : letters first: 1st letter
         * 5 : letters first: 2nd letter
         * 6 : letters first: 1st digit
         * 7 : letters first: 2nd digit */
        int mode;
        char c = toupper(getchar()); /* toupper is there to remove case sensitivity */
        int x_hold = 0;
        int y_hold = 0;

        if (c == EOF) {
                return BUFFER_ERROR;
        }

        /* saving first digit */
        if (isdigit(c)){
                mode = 1;
                y_hold = c - '0';
        } else if (isalpha(c)) {
                mode = 5;
                x_hold = c - 'A' + 1;
        } else {
                return INPUT_ERROR;
        }
        while (!isspace(c = toupper(getchar())) && c != EOF) {
                if (isdigit(c)) {
                        /* skips missing second digit */
                        if (mode == 5) {
                                ++mode;
                        }
                        switch (mode) {
                                case 1:
                                case 7:
                                        /* shifts the old value by a digit, before adding the new value */
                                        y_hold *= 10;
                                        y_hold += c - '0';
                                        ++mode;
                                        break;
                                case 6:
                                        y_hold += c - '0';
                                        ++mode;
                                        break;
                                default:
                                        return INPUT_ERROR;
                        }
                } else if (isalpha(c)) {
                        /* skips missing second letter */
                        if (mode == 1) {
                                ++mode;
                        }
                        switch (mode) {
                                case 2:
                                        x_hold += c - 'A' + 1;
                                        ++mode;
                                        break;
                                case 3:
                                case 5:
                                        /* shifts the old value by a letter, before adding the new value */
                                        x_hold *= 26;
                                        x_hold += c - 'A' + 1;
                                        ++mode;
                                        break;
                                default:
                                        return INPUT_ERROR;
                        }
                } else {
                        return INPUT_ERROR;
                }
        }

        if (c == EOF) {
                return BUFFER_ERROR;
        }

        /* makes sure x_hold, y_hold are within the field */
        if (0 >= x_hold || x_hold > nx || 0 >= y_hold || y_hold > ny) {
                return INPUT_ERROR;
        }
        *x = x_hold - 1;
        *y = y_hold - 1;
        return SUCCESS;
}

/* returns direction (up = 0, left = 1, down = 2, right = 3), passes length on through the pointer or returns negative number in case of error */
int scan_direction(int *length)
{
        int direction = -1;
        int length_hold = -1;
        int i;
        char c;

        for (i = 0; i < 2; ++i) {
                c = toupper(getchar()); /* toupper is there to remove case sensitivity */
                if (c == EOF) {
                        return BUFFER_ERROR;
                }

                switch (c) {
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                                length_hold = c - '0';
                                break;
                        case 'N':
                        case 'U':
                        case '^':
                                direction = 0;
                                break;
                        case 'W':
                        case 'L':
                        case '<':
                                direction = 1;
                                break;
                        case 'S':
                        case 'D':
                        case 'V':
                                direction = 2;
                                break;
                        case 'E':
                        case 'R':
                        case '>':
                                direction = 3;
                                break;
                        default:
                                return INPUT_ERROR;
                }
        }

        if (direction == -1 || length_hold == -1) {
                return INPUT_ERROR;
        }
        *length = length_hold;

        return direction;
}

int choose_ships(play_fields_t *fld)
{
        /* copies of the struct values */
        const int nx = fld->nx;
        const int ny = fld->ny;
        shipstate_t **data_right = fld->data_right;
        /* nShipsRemaining[i] is the number of i long ships left */
        int nShipsRemaining[MAX_SHIP_LENGTH + 1];
        /* using "nRemainingShips" as scratch space */
        memcpy(nShipsRemaining, fld->nShipsRemaining_right, sizeof(nShipsRemaining));

        /* loop-condition checks whether there is still ships, that have not been set */
        while (1) {
                int i, x, y, status, direction, length;
                /* variable containing the whether there are any ships in the way */
                int is_free = TRUE;

                {
                        /* scan to see if any ships are still remaining (not sunk) */
                        int anyShips = FALSE;
                        int i;
                        for (i = MIN_SHIP_LENGTH; i <= MAX_SHIP_LENGTH && !anyShips; ++i) {
                                anyShips = nShipsRemaining[i];
                        }
                        if (!anyShips) {
                                break;
                        }
                }

                print_field(fld);

                printf("You have");
                for (i = MAX_SHIP_LENGTH; i >= MIN_SHIP_LENGTH; --i) {
                        if (nShipsRemaining[i]) {
                                printf(" %i battle ship (length %i)", nShipsRemaining[i], i);
                        }
                }
                printf(" remaining\n");
                printf("Choose where to place your ships, by typing the start and end coordinate of each ship.\n");
                /* Scans coordinate and direction, where ship should be placed */
                while
                (
                    ((status = scan_coordinate(&x, &y, nx, ny)) == INPUT_ERROR && status != BUFFER_ERROR)
                 || ((direction = scan_direction(&length)) == INPUT_ERROR && direction != BUFFER_ERROR)
                )
                {
                        /* an immediate input error? - rescan now */
                        flush_buff();
                        printf("Choose where to place your ships, by typing the start and end coordinate of each ship.\n");
                }
                flush_buff();

                if (status == BUFFER_ERROR) {
                        printf("Buffer Error.\n");
                        return BUFFER_ERROR;
                }

                /* check if there are ships of the selected length left */
                if (length > MAX_SHIP_LENGTH || nShipsRemaining[length] <= 0) {
                        continue;
                }

                /* check the boxes where the ship should be set and its surrounding box for other ships */
                for (i = 0; i < length; ++i) {
                        int x_h = x;
                        int y_h = y;
                        int j;
                        /* in-/decrease one coordinate by i in the direction based on "direction" */
                        switch (direction) {
                                case 0:
                                        y_h = y - i;
                                        break;
                                case 1:
                                        x_h = x - i;
                                        break;
                                case 2:
                                        y_h = y + i;
                                        break;
                                case 3:
                                        x_h = x + i;
                                        break;
                                default:
                                        printf("Something really went wrong in choose_ships(), for(), for(), switch(), default.\n");
                                        return INPUT_ERROR;
                        }
                        /* check whether the points in a 3x3 box around each box, that should be filled, are free */
                        for (j = -1; j <= 1; ++j) {
                                int k;
                                for (k = -1; k <= 1; ++k) {
                                        /* first check if the point is within the field (0 <= point < size) and then whether the the point is occupied */
                                        if (0 <= (y_h + k) && (y_h + k) < ny && 0 <= (x_h + j) && (x_h + j) < nx && *(data_right[y_h + k] + x_h + j) == UNHIT) {
                                                is_free = FALSE;
                                                break;
                                        }
                                }
                                if (!is_free) break;
                        }
                        if (!is_free) break;
                }
                if (!is_free) continue;

                /* places the ship onto the the boxes, by changing their state to UNHIT */
                for (i = 0; i < length; ++i) {
                        /* in-/decreases one coordinate by i in the direction based on "direction" */
                        switch (direction) {
                                case 0:
                                        *(data_right[y - i] + x) = UNHIT;
                                        break;
                                case 1:
                                        *(data_right[y] + x - i) = UNHIT;
                                        break;
                                case 2:
                                        *(data_right[y + i] + x) = UNHIT;
                                        break;
                                case 3:
                                        *(data_right[y] + x + i) = UNHIT;
                                        break;
                                default:
                                        printf("Something really went wrong in choose_ships(), for(), for(), switch(), default.\n");
                                        return INPUT_ERROR;
                        }
                }
                /* decrements the number of ships left of the length "length" */
                --nShipsRemaining[length];
        }

        return SUCCESS;
}

void bot_choose_ships(play_fields_t *fld)
{
        /* copies of the struct values */
        const int nx = fld->nx;
        const int ny = fld->ny;
        shipstate_t **data_left = fld->data_left;

        int length;
        /* num_of_ship_left[i] is the number of i long ships left */
        int nShipsRemaining[MAX_SHIP_LENGTH + 1];
        /* copies the array from the struct in order not to ruin those values */
        memcpy(nShipsRemaining, fld->nShipsRemaining_left, sizeof(nShipsRemaining));

        /* populate with ships - place longest ships first, while there still is enough room */
        for (length = MAX_SHIP_LENGTH; length >= MIN_SHIP_LENGTH; --length) {
                while (nShipsRemaining[length]) {
                        int i;
                        /* ships can only be set up (= 0) or left (= 1) */
                        int direction = rand() % 2;
                        /* changes the possible coordinates, so that the ship fully on the field */
                        int x = direction ? rand() % (nx - (length - 1)) + length - 1 : rand() % nx;
                        int y = direction ? rand() % ny : rand() % (ny - (length - 1)) + length - 1;
                        /* variable containing the whether there are any ships in the way */
                        int is_free = TRUE;

                        /* checks the boxes where the ship should be set and its surrounding box for other ships */
                        for (i = 0; i < length; ++i) {
                                int x_h = x;
                                int y_h = y;
                                int j;
                                /* in-/decreases one coordinate by i in the direction based on "direction" */
                                switch (direction) {
                                        case 0:
                                                y_h = y - i;
                                                break;
                                        case 1:
                                                x_h = x - i;
                                                break;
                                        case 2: case 3:
                                                printf("Directions 2, 3 should not occur in bot_choose_ships().\n");
                                                break;
                                        default:
                                                printf("Something really went wrong in choose_ships(), for(), for(), switch(), default.\n");
                                                return;
                                }
                                if (0 > y_h || y_h >= ny || 0 > x_h || x_h >= nx) {
                                        printf("Error should not be reached!\n");
                                        is_free = FALSE;
                                }
                                /* check whether the points in a 3x3 box around each box, that should be filled, are free */
                                for (j = -1; j <= 1; ++j) {
                                        int k;
                                        for (k = -1; k <= 1; ++k) {
                                                /* first checks if the point is within the field (0 <= point < size) and then whether the the point is occupied */
                                                if (0 <= (y_h + k) && (y_h + k) < ny && 0 <= (x_h + j) && (x_h + j) < nx && *(data_left[y_h + k] + x_h + j) == UNHIT) {
                                                        is_free = FALSE;
                                                        break;
                                                }
                                        }
                                        if (!is_free) break;
                                }
                                if (!is_free) break;
                        }
                        if (!is_free) continue;

                        /* places the ship onto the the boxes, by changing their state to UNHIT */
                        for (i = 0; i < length; ++i) {
                                /* in-/decreases one coordinate by i in the direction based on "direction" */
                                switch (direction) {
                                        case 0:
                                                *(data_left[y - i] + x) = UNHIT;
                                                break;
                                        case 1:
                                                *(data_left[y] + x - i) = UNHIT;
                                                break;
                                        case 2: case 3:
                                                printf("Directions 2, 3 should not occur in bot_choose_ships().\n");
                                                break;
                                        default:
                                                printf("Something really went wrong in bot_choose_ships(), for(), while(), for(), switch(), default.\n");
                                                return;
                                }
                        }
                        /* decrements the number of ships left of the length "length" */
                        --nShipsRemaining[length];
                }
        }
}

int is_already_hit(shipstate_t **battle_field, int x, int y)
{
        return *(battle_field[y] + x) == SPLASH || *(battle_field[y] + x) == HIT || *(battle_field[y] + x) == SUNK;
}

void player_shoot(play_fields_t *fld)
{
        const int nx = fld->nx;
        const int ny = fld->ny;
        shipstate_t **data_left = fld->data_left;

        int x, y;
        int status;

        printf("Type the coordinates, where you want to shoot at.\n");
        while((status = scan_coordinate(&x, &y, nx, ny)) == INPUT_ERROR || is_already_hit(data_left, x, y))  {
                printf("Error: Retype the coordinates, where you want to shoot at.\n");
        }

        switch (*(data_left[y] + x)) {
                case NONE:
                        *(data_left[y] + x) = SPLASH;
                        break;
                case UNHIT:
                        *(data_left[y] + x) = HIT;
                        /* TODO: missing checking for sunk */
                        break;
                default:
                        printf("ERROR: Should not occur - %s line %i\n", __FILE__, __LINE__);
                        return;
        }
}

int main(int argc, char *argv[])
{
        int i;
        /* Default with 10 x 10, default NUM_SHIPS_INIT, NULL pointers instead of arrays, which are initialized in alloc_field() */
        play_fields_t field = {10, 10, NUM_SHIPS_INIT, NUM_SHIPS_INIT, NULL, NULL};

        srand(time(NULL));
        rand();

        /* incomplete, should eventually read the arguments */
        for (i = 1; i < argc; ++i) {
                if ('-' == argv[i][0]) {
                        /*const char *opt = &argv[i][1];
                        printf("process opt: %s\n", argv[i]);
                        if (strncmp(opt, "n=", 2) == 0) {
                                printf("get size = %s\n", argv[i]);
                                field_left.nx = 5;
                                field_right.nx = 5;
                        }*/
                }
        }

        field.data_left = alloc_field(field.nx, field.ny);
        field.data_right = alloc_field(field.nx, field.ny);

        bot_choose_ships(&field);
        /*choose_ships(&field);*/

        for (i = 0; i < 6; ++i) {
                print_field(&field);
                player_shoot(&field);
        }
        free_field(&field);

        return 0;
}

/* ************************************************************************* */
