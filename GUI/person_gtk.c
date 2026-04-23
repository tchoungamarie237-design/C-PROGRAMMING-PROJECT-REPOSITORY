/*
 * person_gtk.c  — WIGMS v4
 * GTK 4  Guest Management
 *
 * Compile:
 *   gcc person_gtk.c -o person_gtk.exe $(pkg-config --cflags --libs gtk4)
 *
 * Changes v4:
 *  - Auto-generated ID is now completely invisible everywhere except the
 *    admin-protected "All Guests" tab (access code: "pokemon").
 *  - The "ID (auto)" label has been removed from the Register tab.
 *  - All other behaviour unchanged from v3.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADMIN_CODE "pokemon"

/* ── Data types ─────────────────────────────── */
typedef enum { LE = 0, LA = 1 } side;
typedef struct {
    int  id;
    char name[100];
    int  age;
    char social_class[50];
    side side;
    int  parking;
} Person;

/* ── CSV helpers ─────────────────────────────── */
#define CSV_FILE   "persons.csv"
#define CSV_HEADER "id,name,age,social_class,side,parking\n"

static void strip_nl(char *s){
    size_t n=strlen(s);
    while(n>0&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]='\0';
}

static void csv_init(void){
    FILE *f=fopen(CSV_FILE,"r");if(f){fclose(f);return;}
    f=fopen(CSV_FILE,"w");if(f){fputs(CSV_HEADER,f);fclose(f);}
}

static void parse_field(char **ptr,char *dst,int maxlen){
    char *p=*ptr;int i=0;
    if(*p=='"'){p++;while(*p&&!(*p=='"'&&(*(p+1)==','||*(p+1)=='\0'))){if(i<maxlen-1)dst[i++]=*p;p++;}if(*p=='"')p++;}
    else{while(*p&&*p!=','){if(i<maxlen-1)dst[i++]=*p;p++;}}
    dst[i]='\0';if(*p==',')p++;*ptr=p;
}

static GArray *csv_load_all(void){
    GArray *arr=g_array_new(FALSE,TRUE,sizeof(Person));
    FILE *f=fopen(CSV_FILE,"r");if(!f)return arr;
    char line[512];fgets(line,sizeof(line),f);
    while(fgets(line,sizeof(line),f)){
        strip_nl(line);if(strlen(line)<3)continue;
        Person p={0};char tmp[256];char *ptr=line;
        parse_field(&ptr,tmp,sizeof(tmp));           p.id=atoi(tmp);
        parse_field(&ptr,p.name,sizeof(p.name));
        parse_field(&ptr,tmp,sizeof(tmp));           p.age=atoi(tmp);
        parse_field(&ptr,p.social_class,sizeof(p.social_class));
        parse_field(&ptr,tmp,sizeof(tmp));           p.side=strcmp(tmp,"LE")==0?LE:LA;
        parse_field(&ptr,tmp,sizeof(tmp));           p.parking=strcmp(tmp,"YES")==0?1:0;
        g_array_append_val(arr,p);
    }
    fclose(f);return arr;
}

static void csv_rewrite_all(GArray *arr){
    FILE *f=fopen(CSV_FILE,"w");if(!f)return;
    fputs(CSV_HEADER,f);
    for(guint i=0;i<arr->len;i++){
        Person *p=&g_array_index(arr,Person,i);
        fprintf(f,"%d,\"%s\",%d,\"%s\",%s,%s\n",
                p->id,p->name,p->age,p->social_class,
                p->side==LE?"LE":"LA",p->parking?"YES":"NO");
    }
    fclose(f);
}

static int csv_next_id(void){
    GArray *arr=csv_load_all();int mx=0;
    for(guint i=0;i<arr->len;i++){int id=g_array_index(arr,Person,i).id;if(id>mx)mx=id;}
    g_array_free(arr,TRUE);return mx+1;
}

static int find_by_name(GArray *arr,const char *name){
    for(guint i=0;i<arr->len;i++)
        if(strcmp(g_array_index(arr,Person,i).name,name)==0)return(int)i;
    return -1;
}

/* ── UI state ────────────────────────────────── */
typedef struct {
    /* Register — no ID widget anymore */
    GtkWidget *entry_name,*entry_age,*entry_class;
    GtkWidget *combo_side,*check_parking;
    GtkWidget *btn_save,*lbl_status;
    /* Update */
    GtkWidget *combo_upd_name;
    GtkWidget *entry_upd_name,*entry_upd_age,*entry_upd_class;
    GtkWidget *combo_upd_side,*check_upd_parking;
    GtkWidget *btn_load,*btn_update,*lbl_upd_status;
    /* Delete */
    GtkWidget *combo_del_name,*btn_delete,*lbl_del_status;
    /* Search (public — no ID) */
    GtkWidget *entry_search,*tree,*lbl_count;
    GtkListStore *store;
    /* Admin list (with ID) */
    GtkWidget *admin_stack;
    GtkWidget *entry_admin_code;
    GtkWidget *lbl_admin_status;
    GtkWidget *admin_tree;
    GtkListStore *admin_store;
    GtkWidget *lbl_admin_count;
    GtkWidget *notebook;
} AppWidgets;

/* Public list columns (no ID) */
enum { COL_NAME=0,COL_AGE,COL_CLASS,COL_SIDE,COL_PARKING,N_COLS };
/* Admin list columns (with ID) */
enum { ACOL_ID=0,ACOL_NAME,ACOL_AGE,ACOL_CLASS,ACOL_SIDE,ACOL_PARKING,ACOL_N };

/* ── Helpers ─────────────────────────────────── */
static void repopulate_combos(AppWidgets *w){
    GArray *arr=csv_load_all();
    GtkStringList *um=gtk_string_list_new(NULL);
    GtkStringList *dm=gtk_string_list_new(NULL);
    for(guint i=0;i<arr->len;i++){
        const char *n=g_array_index(arr,Person,i).name;
        gtk_string_list_append(um,n);
        gtk_string_list_append(dm,n);
    }
    gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_upd_name),G_LIST_MODEL(um));
    gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_del_name),G_LIST_MODEL(dm));
    g_array_free(arr,TRUE);
}

static void refresh_admin_list(AppWidgets *w){
    gtk_list_store_clear(w->admin_store);
    GArray *arr=csv_load_all();
    int park=0;
    for(guint i=0;i<arr->len;i++){
        Person *p=&g_array_index(arr,Person,i);
        park+=p->parking;
        char id_s[16],age_s[16];
        snprintf(id_s,sizeof(id_s),"%d",p->id);
        snprintf(age_s,sizeof(age_s),"%d",p->age);
        GtkTreeIter it;
        gtk_list_store_append(w->admin_store,&it);
        gtk_list_store_set(w->admin_store,&it,
            ACOL_ID,id_s,ACOL_NAME,p->name,ACOL_AGE,age_s,
            ACOL_CLASS,p->social_class,
            ACOL_SIDE,p->side==LE?"LE":"LA",
            ACOL_PARKING,p->parking?"YES":"NO",-1);
    }
    char buf[96];
    snprintf(buf,sizeof(buf),"Total: %d guests  |  Parking: %d",(int)arr->len,park);
    gtk_label_set_text(GTK_LABEL(w->lbl_admin_count),buf);
    g_array_free(arr,TRUE);
}

static void refresh_list(AppWidgets *w,const char *filter){
    gtk_list_store_clear(w->store);
    GArray *arr=csv_load_all();
    int park=0;
    for(guint i=0;i<arr->len;i++){
        Person *p=&g_array_index(arr,Person,i);
        park+=p->parking;
        if(filter&&*filter){
            gchar *ln=g_utf8_strdown(p->name,-1);
            gchar *lc=g_utf8_strdown(p->social_class,-1);
            gchar *lf=g_utf8_strdown(filter,-1);
            gboolean m=strstr(ln,lf)||strstr(lc,lf);
            g_free(ln);g_free(lc);g_free(lf);
            if(!m)continue;
        }
        char age_s[16];snprintf(age_s,sizeof(age_s),"%d",p->age);
        GtkTreeIter it;
        gtk_list_store_append(w->store,&it);
        gtk_list_store_set(w->store,&it,
            COL_NAME,p->name,COL_AGE,age_s,
            COL_CLASS,p->social_class,
            COL_SIDE,p->side==LE?"LE":"LA",
            COL_PARKING,p->parking?"YES":"NO",-1);
    }
    char buf[96];
    snprintf(buf,sizeof(buf),"Total guests: %d  |  With parking: %d",(int)arr->len,park);
    gtk_label_set_text(GTK_LABEL(w->lbl_count),buf);
    g_array_free(arr,TRUE);
    repopulate_combos(w);
}

/* ── Callbacks ───────────────────────────────── */
static void on_save_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    const char *sname=gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    const char *sage =gtk_editable_get_text(GTK_EDITABLE(w->entry_age));
    const char *sclass=gtk_editable_get_text(GTK_EDITABLE(w->entry_class));
    if(!*sname||!*sage||!*sclass){
        gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  Name, Age and Social Class are required.");return;}
    GArray *arr=csv_load_all();
    if(find_by_name(arr,sname)>=0){
        gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  A guest with this name already exists.");
        g_array_free(arr,TRUE);return;}
    g_array_free(arr,TRUE);
    Person p={0};
    p.id=csv_next_id();p.age=atoi(sage);
    p.side=gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_side))==0?LE:LA;
    p.parking=gtk_check_button_get_active(GTK_CHECK_BUTTON(w->check_parking));
    strncpy(p.name,sname,99);strip_nl(p.name);
    strncpy(p.social_class,sclass,49);strip_nl(p.social_class);
    FILE *f=fopen(CSV_FILE,"a");
    if(!f){gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  Cannot write CSV.");return;}
    fprintf(f,"%d,\"%s\",%d,\"%s\",%s,%s\n",
            p.id,p.name,p.age,p.social_class,
            p.side==LE?"LE":"LA",p.parking?"YES":"NO");
    fclose(f);
    gtk_label_set_text(GTK_LABEL(w->lbl_status),"✔  Guest saved successfully.");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),"");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_age),"");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_class),"");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->check_parking),FALSE);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(w->combo_side),0);
    refresh_list(w,NULL);
}

static void on_load_for_update(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_upd_name));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  No guest selected.");return;}
    const char *name=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    GArray *arr=csv_load_all();
    int idx=find_by_name(arr,name);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  Guest not found.");g_array_free(arr,TRUE);return;}
    Person *p=&g_array_index(arr,Person,idx);
    char age_s[16];snprintf(age_s,sizeof(age_s),"%d",p->age);
    gtk_editable_set_text(GTK_EDITABLE(w->entry_upd_name),p->name);
    gtk_editable_set_text(GTK_EDITABLE(w->entry_upd_age),age_s);
    gtk_editable_set_text(GTK_EDITABLE(w->entry_upd_class),p->social_class);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(w->combo_upd_side),p->side==LE?0:1);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->check_upd_parking),p->parking);
    gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"Record loaded — edit then click Update.");
    g_array_free(arr,TRUE);
}

static void on_update_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_upd_name));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  No guest selected.");return;}
    const char *orig=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    const char *sname=gtk_editable_get_text(GTK_EDITABLE(w->entry_upd_name));
    const char *sage =gtk_editable_get_text(GTK_EDITABLE(w->entry_upd_age));
    const char *sclass=gtk_editable_get_text(GTK_EDITABLE(w->entry_upd_class));
    if(!*sname||!*sage||!*sclass){
        gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  All fields required.");return;}
    GArray *arr=csv_load_all();
    int idx=find_by_name(arr,orig);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  Guest not found.");g_array_free(arr,TRUE);return;}
    Person *p=&g_array_index(arr,Person,idx);
    strncpy(p->name,sname,99);strip_nl(p->name);
    strncpy(p->social_class,sclass,49);strip_nl(p->social_class);
    p->age=atoi(sage);
    p->side=gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_upd_side))==0?LE:LA;
    p->parking=gtk_check_button_get_active(GTK_CHECK_BUTTON(w->check_upd_parking));
    csv_rewrite_all(arr);g_array_free(arr,TRUE);
    gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"✔  Guest updated successfully.");
    refresh_list(w,NULL);
}

static void on_delete_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_del_name));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"⚠  No guest selected.");return;}
    const char *name=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    GArray *arr=csv_load_all();
    int idx=find_by_name(arr,name);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"⚠  Guest not found.");g_array_free(arr,TRUE);return;}
    g_array_remove_index(arr,idx);
    csv_rewrite_all(arr);g_array_free(arr,TRUE);
    gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"✔  Guest deleted successfully.");
    refresh_list(w,NULL);
}

static void on_search_changed(GtkEditable *e,gpointer data){
    refresh_list((AppWidgets*)data,gtk_editable_get_text(e));
}

static void on_admin_unlock(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    const char *code=gtk_editable_get_text(GTK_EDITABLE(w->entry_admin_code));
    if(strcmp(code,ADMIN_CODE)==0){
        gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"list");
        refresh_admin_list(w);
        gtk_editable_set_text(GTK_EDITABLE(w->entry_admin_code),"");
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_admin_status),"⚠  Incorrect access code.");
    }
}

static void on_admin_lock(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"lock");
}

/* ── Build tabs ──────────────────────────────── */

/* Register tab — ID completely hidden, no label at all */
static GtkWidget *build_register_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);
    gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;

#define AR(l,ww) gtk_grid_attach(GTK_GRID(grid),gtk_label_new(l),0,row,1,1);\
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);\
    gtk_grid_attach(GTK_GRID(grid),ww,1,row,2,1);gtk_widget_set_hexpand(ww,TRUE);row++;

    /* No ID row — ID is generated silently in on_save_clicked */
    w->entry_name=gtk_entry_new();  gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name),"Full name");
    w->entry_age=gtk_entry_new();   gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_age),"e.g. 30");
    w->entry_class=gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_class),"e.g. VIP");
    AR("Name",w->entry_name)
    AR("Age",w->entry_age)
    AR("Social Class",w->entry_class)

    const char *sides[]={"LE","LA",NULL};
    w->combo_side=gtk_drop_down_new_from_strings(sides);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Side"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_side,1,row,2,1);row++;

    w->check_parking=gtk_check_button_new_with_label("Needs parking spot");
    gtk_grid_attach(GTK_GRID(grid),w->check_parking,1,row,2,1);row++;

    w->btn_save=gtk_button_new_with_label("💾  Save Guest");
    gtk_grid_attach(GTK_GRID(grid),w->btn_save,0,row,3,1);row++;
    w->lbl_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_status,0,row,3,1);
    g_signal_connect(w->btn_save,"clicked",G_CALLBACK(on_save_clicked),w);
#undef AR
    return grid;
}

static GtkWidget *build_update_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;
#define AR(l,ww) gtk_grid_attach(GTK_GRID(grid),gtk_label_new(l),0,row,1,1);\
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);\
    gtk_grid_attach(GTK_GRID(grid),ww,1,row,2,1);gtk_widget_set_hexpand(ww,TRUE);row++;

    w->combo_upd_name=gtk_drop_down_new(NULL,NULL);gtk_widget_set_hexpand(w->combo_upd_name,TRUE);
    w->btn_load=gtk_button_new_with_label("🔍  Load");
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Select guest"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_upd_name,1,row,1,1);
    gtk_grid_attach(GTK_GRID(grid),w->btn_load,2,row,1,1);row++;

    w->entry_upd_name=gtk_entry_new();  gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_upd_name),"Full name");
    w->entry_upd_age=gtk_entry_new();   gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_upd_age),"Age");
    w->entry_upd_class=gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_upd_class),"Social class");
    AR("Name",w->entry_upd_name) AR("Age",w->entry_upd_age) AR("Social Class",w->entry_upd_class)

    const char *sides[]={"LE","LA",NULL};
    w->combo_upd_side=gtk_drop_down_new_from_strings(sides);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Side"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_upd_side,1,row,2,1);row++;

    w->check_upd_parking=gtk_check_button_new_with_label("Needs parking spot");
    gtk_grid_attach(GTK_GRID(grid),w->check_upd_parking,1,row,2,1);row++;

    w->btn_update=gtk_button_new_with_label("✏️  Update Guest");
    gtk_grid_attach(GTK_GRID(grid),w->btn_update,0,row,3,1);row++;
    w->lbl_upd_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_upd_status,0,row,3,1);
    g_signal_connect(w->btn_load,"clicked",G_CALLBACK(on_load_for_update),w);
    g_signal_connect(w->btn_update,"clicked",G_CALLBACK(on_update_clicked),w);
#undef AR
    return grid;
}

static GtkWidget *build_delete_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;
    w->combo_del_name=gtk_drop_down_new(NULL,NULL);gtk_widget_set_hexpand(w->combo_del_name,TRUE);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Select guest"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_del_name,1,row,2,1);row++;
    w->btn_delete=gtk_button_new_with_label("🗑  Delete Guest");
    gtk_grid_attach(GTK_GRID(grid),w->btn_delete,0,row,3,1);row++;
    w->lbl_del_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_del_status,0,row,3,1);
    g_signal_connect(w->btn_delete,"clicked",G_CALLBACK(on_delete_clicked),w);
    return grid;
}

static GtkWidget *build_search_tab(AppWidgets *w){
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(vbox,16);gtk_widget_set_margin_end(vbox,16);
    gtk_widget_set_margin_top(vbox,16);gtk_widget_set_margin_bottom(vbox,16);
    w->entry_search=gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(w->entry_search),"Search by name or social class…");
    gtk_box_append(GTK_BOX(vbox),w->entry_search);
    g_signal_connect(w->entry_search,"search-changed",G_CALLBACK(on_search_changed),w);
    w->lbl_count=gtk_label_new("Total guests: 0  |  With parking: 0");
    gtk_widget_set_halign(w->lbl_count,GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox),w->lbl_count);
    w->store=gtk_list_store_new(N_COLS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    w->tree=gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w->tree),TRUE);
    const char *titles[N_COLS]={"Name","Age","Social Class","Side","Parking"};
    for(int c=0;c<N_COLS;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(titles[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);gtk_tree_view_column_set_sort_column_id(col,c);
        gtk_tree_view_append_column(GTK_TREE_VIEW(w->tree),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),w->tree);
    gtk_box_append(GTK_BOX(vbox),scroll);
    return vbox;
}

/* Admin-only All Guests tab — ID visible, protected by "pokemon" */
static GtkWidget *build_admin_list_tab(AppWidgets *w){
    w->admin_stack=gtk_stack_new();

    /* Lock page */
    GtkWidget *lock_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);
    gtk_widget_set_margin_start(lock_box,60);gtk_widget_set_margin_end(lock_box,60);
    gtk_widget_set_margin_top(lock_box,60);gtk_widget_set_margin_bottom(lock_box,60);
    GtkWidget *lbl_title=gtk_label_new("🔒  Admin Access");
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.4));
    gtk_label_set_attributes(GTK_LABEL(lbl_title),a);pango_attr_list_unref(a);
    gtk_widget_set_halign(lbl_title,GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(lock_box),lbl_title);
    GtkWidget *lbl_hint=gtk_label_new("This section is reserved for the Administrator.\nEnter the access code to continue.");
    gtk_label_set_justify(GTK_LABEL(lbl_hint),GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(lock_box),lbl_hint);
    w->entry_admin_code=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_admin_code),"Access code");
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_admin_code),FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(w->entry_admin_code),GTK_INPUT_PURPOSE_PASSWORD);
    gtk_box_append(GTK_BOX(lock_box),w->entry_admin_code);
    GtkWidget *btn_unlock=gtk_button_new_with_label("Unlock");
    gtk_box_append(GTK_BOX(lock_box),btn_unlock);
    w->lbl_admin_status=gtk_label_new("");
    gtk_box_append(GTK_BOX(lock_box),w->lbl_admin_status);
    g_signal_connect(btn_unlock,"clicked",G_CALLBACK(on_admin_unlock),w);
    g_signal_connect_swapped(w->entry_admin_code,"activate",G_CALLBACK(on_admin_unlock),w);
    gtk_stack_add_named(GTK_STACK(w->admin_stack),lock_box,"lock");

    /* List page */
    GtkWidget *list_vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(list_vbox,16);gtk_widget_set_margin_end(list_vbox,16);
    gtk_widget_set_margin_top(list_vbox,16);gtk_widget_set_margin_bottom(list_vbox,16);
    GtkWidget *top_row=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
    w->lbl_admin_count=gtk_label_new("Total: 0");
    gtk_widget_set_halign(w->lbl_admin_count,GTK_ALIGN_START);
    gtk_widget_set_hexpand(w->lbl_admin_count,TRUE);
    gtk_box_append(GTK_BOX(top_row),w->lbl_admin_count);
    GtkWidget *btn_lock=gtk_button_new_with_label("🔒  Lock");
    g_signal_connect(btn_lock,"clicked",G_CALLBACK(on_admin_lock),w);
    gtk_box_append(GTK_BOX(top_row),btn_lock);
    gtk_box_append(GTK_BOX(list_vbox),top_row);
    w->admin_store=gtk_list_store_new(ACOL_N,
        G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,
        G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    w->admin_tree=gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->admin_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w->admin_tree),TRUE);
    const char *titles[ACOL_N]={"ID","Name","Age","Social Class","Side","Parking"};
    for(int c=0;c<ACOL_N;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(titles[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);gtk_tree_view_column_set_sort_column_id(col,c);
        gtk_tree_view_append_column(GTK_TREE_VIEW(w->admin_tree),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),w->admin_tree);
    gtk_box_append(GTK_BOX(list_vbox),scroll);
    gtk_stack_add_named(GTK_STACK(w->admin_stack),list_vbox,"list");
    gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"lock");
    return w->admin_stack;
}

static void activate(GtkApplication *app,gpointer user_data){
    (void)user_data;csv_init();
    AppWidgets *w=g_new0(AppWidgets,1);
    GtkWidget *window=gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),"WIGMS — Guest Management");
    gtk_window_set_default_size(GTK_WINDOW(window),800,560);
    GtkWidget *nb=gtk_notebook_new();
    w->notebook=nb;
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_register_tab(w), gtk_label_new("📝  Register"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_update_tab(w),   gtk_label_new("✏️  Update"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_delete_tab(w),   gtk_label_new("🗑  Delete"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_search_tab(w),   gtk_label_new("🔍  Search"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_admin_list_tab(w),gtk_label_new("🔒  All Guests"));
    gtk_window_set_child(GTK_WINDOW(window),nb);
    refresh_list(w,NULL);
    gtk_widget_set_visible(window,TRUE);
}

int main(int argc,char **argv){
    GtkApplication *app=gtk_application_new("cm.wigms.person",G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
    int s=g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);return s;
}
