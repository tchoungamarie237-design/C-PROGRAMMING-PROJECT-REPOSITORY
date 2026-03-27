/*
 * wigms_gtk.c
 * GTK 4 — Person Registration System with CSV storage
 *
 * Compile:
 *   gcc wigms_gtk.c -o wigms_gtk $(pkg-config --cflags --libs gtk4) -lm
 *
 * CSV file: persons.csv  (auto-created next to the executable)
 * Columns : id,name,age,social_class,side
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────
   Data types  (unchanged from your original)
───────────────────────────────────────────── */
typedef enum { LE = 0, LA = 1 } side;

typedef struct {
    int  id;
    char name[100];
    int  age;
    char social_class[50];
    side side;
} Person;

/* ─────────────────────────────────────────────
   CSV helpers
───────────────────────────────────────────── */
#define CSV_FILE "persons.csv"
#define CSV_HEADER "id,name,age,social_class,side\n"

/* Strip trailing newline / carriage return */
static void strip_nl(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

/* Ensure the CSV file exists with a header */
static void csv_init(void) {
    FILE *f = fopen(CSV_FILE, "r");
    if (f) { fclose(f); return; }
    f = fopen(CSV_FILE, "w");
    if (f) { fputs(CSV_HEADER, f); fclose(f); }
}

/* Append one person to the CSV */
static gboolean csv_save_person(const Person *p, char *err_out) {
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) { snprintf(err_out, 256, "Cannot open %s for writing.", CSV_FILE); return FALSE; }
    /* Escape commas in name / social_class with quotes */
    fprintf(f, "%d,\"%s\",%d,\"%s\",%s\n",
            p->id, p->name, p->age, p->social_class,
            p->side == LE ? "LE" : "LA");
    fclose(f);
    return TRUE;
}

/*
 * Load all persons from CSV into a GArray.
 * Caller must call g_array_free(arr, TRUE) when done.
 */
static GArray *csv_load_all(void) {
    GArray *arr = g_array_new(FALSE, TRUE, sizeof(Person));
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) return arr;

    char line[512];
    fgets(line, sizeof(line), f); /* skip header */

    while (fgets(line, sizeof(line), f)) {
        strip_nl(line);
        if (strlen(line) < 3) continue;

        Person p = {0};
        /* Simple tokeniser that respects quoted fields */
        char *ptr = line;
        char fields[5][256];
        int fi = 0;

        for (; fi < 5 && *ptr; fi++) {
            char *dst = fields[fi];
            if (*ptr == '"') {
                ptr++; /* skip opening quote */
                while (*ptr && !(*ptr == '"' && *(ptr+1) == ',')
                            && !(*ptr == '"' && *(ptr+1) == '\0')) {
                    *dst++ = *ptr++;
                }
                if (*ptr == '"') ptr++; /* skip closing quote */
            } else {
                while (*ptr && *ptr != ',') *dst++ = *ptr++;
            }
            *dst = '\0';
            if (*ptr == ',') ptr++;
        }

        p.id   = atoi(fields[0]);
        strncpy(p.name,         fields[1], 99);
        p.age  = atoi(fields[2]);
        strncpy(p.social_class, fields[3], 49);
        p.side = (strcmp(fields[4], "LE") == 0) ? LE : LA;

        g_array_append_val(arr, p);
    }
    fclose(f);
    return arr;
}

/* ─────────────────────────────────────────────
   UI state (all widgets we need across callbacks)
───────────────────────────────────────────── */
typedef struct {
    /* Registration tab */
    GtkWidget *entry_id;
    GtkWidget *entry_name;
    GtkWidget *entry_age;
    GtkWidget *entry_class;
    GtkWidget *combo_side;
    GtkWidget *btn_save;
    GtkWidget *lbl_status;

    /* List/Search tab */
    GtkWidget *entry_search;
    GtkListStore *store;
    GtkWidget *tree;
} AppWidgets;

/* ─────────────────────────────────────────────
   List-store columns
───────────────────────────────────────────── */
enum {
    COL_ID = 0,
    COL_NAME,
    COL_AGE,
    COL_CLASS,
    COL_SIDE,
    N_COLS
};

/* ─────────────────────────────────────────────
   Populate / refresh the tree view
───────────────────────────────────────────── */
static void refresh_list(AppWidgets *w, const char *filter) {
    GListStore(w->store);

    GArray *arr = csv_load_all();
    for (guint i = 0; i < arr->len; i++) {
        Person *p = &g_array_index(arr, Person, i);

        /* Filter: case-insensitive substring on name or social_class */
        if (filter && *filter) {
            gchar *lo_name  = g_utf8_strdown(p->name,         -1);
            gchar *lo_class = g_utf8_strdown(p->social_class, -1);
            gchar *lo_filt  = g_utf8_strdown(filter,          -1);
            gboolean match  = strstr(lo_name,  lo_filt) != NULL
                           || strstr(lo_class, lo_filt) != NULL;
            g_free(lo_name); g_free(lo_class); g_free(lo_filt);
            if (!match) continue;
        }

        char id_str[16], age_str[16];
        snprintf(id_str,  sizeof(id_str),  "%d", p->id);
        snprintf(age_str, sizeof(age_str), "%d", p->age);

        GtkTreeIter iter;
        GListStore(w->store, &iter);
        GListStore(w->store, &iter,
            COL_ID,    id_str,
            COL_NAME,  p->name,
            COL_AGE,   age_str,
            COL_CLASS, p->social_class,
            COL_SIDE,  p->side == LE ? "LE" : "LA",
            -1);
    }
    g_array_free(arr, TRUE);
}

/* ─────────────────────────────────────────────
   Callbacks
───────────────────────────────────────────── */
static void on_save_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    AppWidgets *w = (AppWidgets *)data;

    const char *sid   = gtk_editable_get_text(GTK_EDITABLE(w->entry_id));
    const char *sname = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    const char *sage  = gtk_editable_get_text(GTK_EDITABLE(w->entry_age));
    const char *sclass= gtk_editable_get_text(GTK_EDITABLE(w->entry_class));
    int side_sel      = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_side));

    /* Basic validation */
    if (!*sid || !*sname || !*sage || !*sclass) {
        gtk_label_set_text(GTK_LABEL(w->lbl_status), "⚠  All fields are required.");
        return;
    }

    Person p = {0};
    p.id  = atoi(sid);
    p.age = atoi(sage);
    strncpy(p.name,         sname,  99);
    strncpy(p.social_class, sclass, 49);
    p.side = (side_sel == 0) ? LE : LA;

    /* Strip any trailing newline that fgets would have added */
    strip_nl(p.name);
    strip_nl(p.social_class);

    char err[256] = {0};
    if (!csv_save_person(&p, err)) {
        gtk_label_set_text(GTK_LABEL(w->lbl_status), err);
        return;
    }

    gtk_label_set_text(GTK_LABEL(w->lbl_status), "✔  Person saved successfully.");

    /* Clear fields */
    gtk_editable_set_text(GTK_EDITABLE(w->entry_id),    "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),  "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_age),   "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_class), "");
    gtk_drop_down_set_selected(GTK_DROP_DOWN(w->combo_side), 0);

    /* Refresh list */
    refresh_list(w, NULL);
}

static void on_search_changed(GtkEditable *editable, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;
    const char *text = gtk_editable_get_text(editable);
    refresh_list(w, text);
}

/* ─────────────────────────────────────────────
   Build UI
───────────────────────────────────────────── */
static GtkWidget *build_register_tab(AppWidgets *w) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing   (GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start (grid, 20);
    gtk_widget_set_margin_end   (grid, 20);
    gtk_widget_set_margin_top   (grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);

    int row = 0;

    /* Helper macro */
#define ADD_ROW(label_text, entry_widget) \
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new(label_text), 0, row, 1, 1); \
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid), 0, row), GTK_ALIGN_START); \
    gtk_grid_attach(GTK_GRID(grid), entry_widget, 1, row, 2, 1); \
    gtk_widget_set_hexpand(entry_widget, TRUE); \
    row++;

    w->entry_id    = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_id),    "e.g. 42");
    w->entry_name  = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name),  "Full name");
    w->entry_age   = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_age),   "e.g. 30");
    w->entry_class = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_class), "e.g. VIP");

    ADD_ROW("ID",           w->entry_id);
    ADD_ROW("Name",         w->entry_name);
    ADD_ROW("Age",          w->entry_age);
    ADD_ROW("Social Class", w->entry_class);

    /* Side dropdown */
    const char *sides[] = { "LE", "LA", NULL };
    w->combo_side = gtk_drop_down_new_from_strings(sides);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Side"), 0, row, 1, 1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid), 0, row), GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), w->combo_side, 1, row, 2, 1);
    row++;

    /* Save button */
    w->btn_save = gtk_button_new_with_label("💾  Save Person");
    gtk_grid_attach(GTK_GRID(grid), w->btn_save, 0, row, 3, 1);
    row++;

    /* Status label */
    w->lbl_status = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), w->lbl_status, 0, row, 3, 1);

    g_signal_connect(w->btn_save, "clicked", G_CALLBACK(on_save_clicked), w);

    return grid;
}

static GtkWidget *build_list_tab(AppWidgets *w) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start (vbox, 16);
    gtk_widget_set_margin_end   (vbox, 16);
    gtk_widget_set_margin_top   (vbox, 16);
    gtk_widget_set_margin_bottom(vbox, 16);

    /* Search bar */
    w->entry_search = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(w->entry_search),
                                          "Search by name or social class…");
    gtk_box_append(GTK_BOX(vbox), w->entry_search);
    g_signal_connect(w->entry_search, "search-changed", G_CALLBACK(on_search_changed), w);

    /* List store */
    w->store = GListStore(N_COLS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING);

    /* Tree view */
    w->tree = GtkListView(GTK_TREE_MODEL(w->store));
    GtkListView(GTK_TREE_VIEW(w->tree), TRUE);

    const char *titles[N_COLS] = { "ID", "Name", "Age", "Social Class", "Side" };
    for (int c = 0; c < N_COLS; c++) {
        GtkCellRenderer   *rend = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col  = GtkColumn(
                                      titles[c], rend, "text", c, NULL);
        GtkColumnView and GtkColumnViewColumn(col, TRUE);
        GtkColumnView and GtkColumnViewColumn(col, c);
        GtkListView(w->tree), col);
    }

    /* Scrolled window */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), w->tree);
    gtk_box_append(GTK_BOX(vbox), scroll);

    return vbox;
}

/* ─────────────────────────────────────────────
   App activate
───────────────────────────────────────────── */
static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    csv_init();

    AppWidgets *w = g_new0(AppWidgets, 1);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "WIGMS — Person Registration");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);

    /* Notebook (tabs) */
    GtkWidget *notebook = gtk_notebook_new();

    GtkWidget *reg_tab  = build_register_tab(w);
    GtkWidget *list_tab = build_list_tab(w);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), reg_tab,
                             gtk_label_new("📝  Register"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), list_tab,
                             gtk_label_new("📋  All Persons"));

    gtk_window_set_child(GTK_WINDOW(window), notebook);

    /* Initial list load */
    refresh_list(w, NULL);

    gtk_widget_set_visible(window, TRUE);
}

/* ─────────────────────────────────────────────
   Main 1
───────────────────────────────────────────── */
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("cm.wigms.app",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
