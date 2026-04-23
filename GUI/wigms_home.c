/*
 * wigms_home.c  — WIGMS v4
 * GTK 4  Home Launcher + Admin Dashboard
 *
 * Compile:
 *   gcc wigms_home.c -o wigms_home.exe $(pkg-config --cflags --libs gtk4)
 *
 * Changes v4:
 *  - "Open Category Manager" button is now admin-protected.
 *    Clicking it shows a password dialog; category_gtk.exe only launches
 *    if the user enters "pokemon" correctly.
 *  - Dashboard tab still protected by "pokemon 1" (unchanged).
 *  - Guest Manager remains public (no password required).
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DASH_CODE "pokemon 1"
#define CAT_CODE  "pokemon"

/* ── Minimal CSV readers ─────────────────────── */
typedef enum { LE=0,LA=1 } side;
typedef struct { int id;char name[100];int age;char social_class[50];side side;int parking; } Person;
typedef struct { int id;char code[50];char guest_name[4][100]; } Category;

static void strip_nl(char *s){size_t n=strlen(s);while(n>0&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]='\0';}
static void parse_field(char **ptr,char *dst,int maxlen){
    char *p=*ptr;int i=0;
    if(*p=='"'){p++;while(*p&&!(*p=='"'&&(*(p+1)==','||*(p+1)=='\0'))){if(i<maxlen-1)dst[i++]=*p;p++;}if(*p=='"')p++;}
    else{while(*p&&*p!=','){if(i<maxlen-1)dst[i++]=*p;p++;}}
    dst[i]='\0';if(*p==',')p++;*ptr=p;
}

static GArray *load_persons(void){
    GArray *a=g_array_new(FALSE,TRUE,sizeof(Person));
    FILE *f=fopen("persons.csv","r");if(!f)return a;
    char line[512];fgets(line,sizeof(line),f);
    while(fgets(line,sizeof(line),f)){
        strip_nl(line);if(strlen(line)<3)continue;
        Person p={0};char tmp[256];char *ptr=line;
        parse_field(&ptr,tmp,sizeof(tmp));         p.id=atoi(tmp);
        parse_field(&ptr,p.name,sizeof(p.name));
        parse_field(&ptr,tmp,sizeof(tmp));         p.age=atoi(tmp);
        parse_field(&ptr,p.social_class,sizeof(p.social_class));
        parse_field(&ptr,tmp,sizeof(tmp));         p.side=strcmp(tmp,"LE")==0?LE:LA;
        parse_field(&ptr,tmp,sizeof(tmp));         p.parking=strcmp(tmp,"YES")==0?1:0;
        g_array_append_val(a,p);
    }
    fclose(f);return a;
}

static GArray *load_categories(void){
    GArray *a=g_array_new(FALSE,TRUE,sizeof(Category));
    FILE *f=fopen("categories.csv","r");if(!f)return a;
    char line[1024];fgets(line,sizeof(line),f);
    while(fgets(line,sizeof(line),f)){
        strip_nl(line);if(strlen(line)<2)continue;
        Category c={0};char tmp[64];char *ptr=line;
        parse_field(&ptr,tmp,sizeof(tmp));c.id=atoi(tmp);
        parse_field(&ptr,c.code,sizeof(c.code));
        for(int g=0;g<4;g++)parse_field(&ptr,c.guest_name[g],100);
        g_array_append_val(a,c);
    }
    fclose(f);return a;
}

/* ── App launcher ────────────────────────────── */
static void do_launch(const char *exe, GtkWidget *status_lbl){
    char cmd[512];
#ifdef _WIN32
    snprintf(cmd,sizeof(cmd),"start \"\" \"%s\"",exe);
#else
    snprintf(cmd,sizeof(cmd),"./%s &",exe);
#endif
    char msg[256];
    if(system(cmd)!=0) snprintf(msg,sizeof(msg),"⚠  Could not launch %s",exe);
    else               snprintf(msg,sizeof(msg),"✔  %s launched.",exe);
    gtk_label_set_text(GTK_LABEL(status_lbl),msg);
}

/* ── Simple launch (Guest Manager — public) ─── */
typedef struct { const char *exe; GtkWidget *status_lbl; } LaunchData;
static void on_btn_launch_public(GtkButton *btn,gpointer data){
    (void)btn;LaunchData *ld=(LaunchData*)data;
    do_launch(ld->exe,ld->status_lbl);
}

/* ── Password dialog for Category Manager ────── */
typedef struct {
    GtkWidget  *dialog;
    GtkWidget  *entry;
    GtkWidget  *lbl_error;
    GtkWidget  *status_lbl;   /* home page status label */
} CatDialog;

static void on_cat_dialog_unlock(GtkButton *btn,gpointer data){
    (void)btn;CatDialog *cd=(CatDialog*)data;
    const char *code=gtk_editable_get_text(GTK_EDITABLE(cd->entry));
    if(strcmp(code,CAT_CODE)==0){
        gtk_window_close(GTK_WINDOW(cd->dialog));
        do_launch("category_gtk.exe",cd->status_lbl);
    } else {
        gtk_label_set_text(GTK_LABEL(cd->lbl_error),"⚠  Incorrect access code. Try again.");
        gtk_editable_set_text(GTK_EDITABLE(cd->entry),"");
    }
}

static void on_cat_dialog_cancel(GtkButton *btn,gpointer data){
    (void)btn;CatDialog *cd=(CatDialog*)data;
    gtk_window_close(GTK_WINDOW(cd->dialog));
}

static void on_btn_launch_category(GtkButton *btn,gpointer data){
    (void)btn;
    GtkWidget *status_lbl=(GtkWidget*)data;

    /* Build a modal password window */
    GtkWidget *dialog=gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog),"Category Manager — Admin Access");
    gtk_window_set_default_size(GTK_WINDOW(dialog),400,260);
    gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog),FALSE);

    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);
    gtk_widget_set_margin_start(vbox,40);gtk_widget_set_margin_end(vbox,40);
    gtk_widget_set_margin_top(vbox,40);gtk_widget_set_margin_bottom(vbox,40);

    /* Lock icon + title */
    GtkWidget *title=gtk_label_new("🔒  Admin Access Required");
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(title),a);pango_attr_list_unref(a);
    gtk_widget_set_halign(title,GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox),title);

    GtkWidget *hint=gtk_label_new("The Category Manager is reserved for the Admin.\nEnter the access code to open it.");
    gtk_label_set_justify(GTK_LABEL(hint),GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox),hint);

    /* Password entry */
    GtkWidget *entry=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Access code");
    gtk_entry_set_visibility(GTK_ENTRY(entry),FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(entry),GTK_INPUT_PURPOSE_PASSWORD);
    gtk_box_append(GTK_BOX(vbox),entry);

    /* Error label */
    GtkWidget *lbl_error=gtk_label_new("");
    gtk_box_append(GTK_BOX(vbox),lbl_error);

    /* Button row */
    GtkWidget *btn_row=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
    gtk_widget_set_halign(btn_row,GTK_ALIGN_CENTER);
    GtkWidget *btn_cancel=gtk_button_new_with_label("Cancel");
    GtkWidget *btn_ok=gtk_button_new_with_label("Open Category Manager");
    gtk_box_append(GTK_BOX(btn_row),btn_cancel);
    gtk_box_append(GTK_BOX(btn_row),btn_ok);
    gtk_box_append(GTK_BOX(vbox),btn_row);

    gtk_window_set_child(GTK_WINDOW(dialog),vbox);

    /* Wire up dialog state */
    CatDialog *cd=g_new0(CatDialog,1);
    cd->dialog=dialog;
    cd->entry=entry;
    cd->lbl_error=lbl_error;
    cd->status_lbl=status_lbl;

    g_signal_connect(btn_ok,    "clicked",G_CALLBACK(on_cat_dialog_unlock),cd);
    g_signal_connect(btn_cancel,"clicked",G_CALLBACK(on_cat_dialog_cancel),cd);
    /* Also trigger unlock on Enter key */
    g_signal_connect_swapped(entry,"activate",G_CALLBACK(on_cat_dialog_unlock),cd);
    /* Free CatDialog struct when dialog is destroyed */
    g_signal_connect_swapped(dialog,"destroy",G_CALLBACK(g_free),cd);

    gtk_widget_set_visible(dialog,TRUE);
}

/* ── Build home card ─────────────────────────── */
static GtkWidget *build_card_public(const char *title,const char *subtitle,
                                     const char *btn_lbl,const char *exe,
                                     GtkWidget *status_lbl){
    GtkWidget *frame=gtk_frame_new(NULL);gtk_widget_set_hexpand(frame,TRUE);
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
    gtk_widget_set_margin_start(vbox,24);gtk_widget_set_margin_end(vbox,24);
    gtk_widget_set_margin_top(vbox,20);gtk_widget_set_margin_bottom(vbox,20);
    GtkWidget *lt=gtk_label_new(title);gtk_widget_set_halign(lt,GTK_ALIGN_START);
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lt),a);pango_attr_list_unref(a);
    gtk_box_append(GTK_BOX(vbox),lt);
    GtkWidget *ls=gtk_label_new(subtitle);gtk_widget_set_halign(ls,GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(ls),TRUE);gtk_box_append(GTK_BOX(vbox),ls);
    gtk_box_append(GTK_BOX(vbox),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *btn=gtk_button_new_with_label(btn_lbl);
    LaunchData *ld=g_new0(LaunchData,1);ld->exe=exe;ld->status_lbl=status_lbl;
    g_signal_connect(btn,"clicked",G_CALLBACK(on_btn_launch_public),ld);
    gtk_box_append(GTK_BOX(vbox),btn);
    gtk_frame_set_child(GTK_FRAME(frame),vbox);
    return frame;
}

static GtkWidget *build_card_protected(const char *title,const char *subtitle,
                                        const char *btn_lbl,GtkWidget *status_lbl){
    GtkWidget *frame=gtk_frame_new(NULL);gtk_widget_set_hexpand(frame,TRUE);
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
    gtk_widget_set_margin_start(vbox,24);gtk_widget_set_margin_end(vbox,24);
    gtk_widget_set_margin_top(vbox,20);gtk_widget_set_margin_bottom(vbox,20);
    GtkWidget *lt=gtk_label_new(title);gtk_widget_set_halign(lt,GTK_ALIGN_START);
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lt),a);pango_attr_list_unref(a);
    gtk_box_append(GTK_BOX(vbox),lt);
    GtkWidget *ls=gtk_label_new(subtitle);gtk_widget_set_halign(ls,GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(ls),TRUE);gtk_box_append(GTK_BOX(vbox),ls);
    gtk_box_append(GTK_BOX(vbox),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    /* Small lock badge below separator */
    GtkWidget *badge=gtk_label_new("🔒  Admin only");
    gtk_widget_set_halign(badge,GTK_ALIGN_START);
    PangoAttrList *b=pango_attr_list_new();
    pango_attr_list_insert(b,pango_attr_scale_new(0.85));
    gtk_label_set_attributes(GTK_LABEL(badge),b);pango_attr_list_unref(b);
    gtk_box_append(GTK_BOX(vbox),badge);
    GtkWidget *btn=gtk_button_new_with_label(btn_lbl);
    g_signal_connect(btn,"clicked",G_CALLBACK(on_btn_launch_category),status_lbl);
    gtk_box_append(GTK_BOX(vbox),btn);
    gtk_frame_set_child(GTK_FRAME(frame),vbox);
    return frame;
}

/* ── Dashboard state ─────────────────────────── */
typedef struct {
    GtkWidget *dash_stack;
    GtkWidget *entry_dash_code;
    GtkWidget *lbl_dash_lock_status;
    GtkWidget *lbl_total_guests;
    GtkWidget *lbl_total_cats;
    GtkWidget *lbl_le,*lbl_la;
    GtkWidget *lbl_parking;
    GtkWidget *lbl_assigned,*lbl_unassigned;
    GtkListStore *store_cats;
    GtkWidget *tree_cats;
    GtkWidget *btn_refresh;
} DashState;

enum { DASH_CODE_COL=0,DASH_G0,DASH_G1,DASH_G2,DASH_G3,DASH_N };

static void dashboard_refresh(GtkButton *btn,gpointer data){
    (void)btn;DashState *d=(DashState*)data;
    GArray *persons=load_persons();
    GArray *cats=load_categories();
    int total_p=(int)persons->len,total_c=(int)cats->len;
    int le=0,la=0,park=0;
    for(guint i=0;i<persons->len;i++){
        Person *p=&g_array_index(persons,Person,i);
        if(p->side==LE)le++;else la++;park+=p->parking;
    }
    int assigned=0;
    for(guint i=0;i<cats->len;i++){
        Category *c=&g_array_index(cats,Category,i);
        for(int g=0;g<4;g++) if(strlen(c->guest_name[g])>0) assigned++;
    }
    int unassigned=total_p-assigned;if(unassigned<0)unassigned=0;
    char buf[128];
    snprintf(buf,sizeof(buf),"%d guests registered",total_p);      gtk_label_set_text(GTK_LABEL(d->lbl_total_guests),buf);
    snprintf(buf,sizeof(buf),"%d categories",total_c);             gtk_label_set_text(GTK_LABEL(d->lbl_total_cats),buf);
    snprintf(buf,sizeof(buf),"Side LE: %d",le);                    gtk_label_set_text(GTK_LABEL(d->lbl_le),buf);
    snprintf(buf,sizeof(buf),"Side LA: %d",la);                    gtk_label_set_text(GTK_LABEL(d->lbl_la),buf);
    snprintf(buf,sizeof(buf),"Parking requests: %d",park);         gtk_label_set_text(GTK_LABEL(d->lbl_parking),buf);
    snprintf(buf,sizeof(buf),"Assigned to category: %d",assigned); gtk_label_set_text(GTK_LABEL(d->lbl_assigned),buf);
    snprintf(buf,sizeof(buf),"Not yet assigned: %d",unassigned);   gtk_label_set_text(GTK_LABEL(d->lbl_unassigned),buf);
    gtk_list_store_clear(d->store_cats);
    for(guint i=0;i<cats->len;i++){
        Category *c=&g_array_index(cats,Category,i);
        GtkTreeIter it;gtk_list_store_append(d->store_cats,&it);
        gtk_list_store_set(d->store_cats,&it,
            DASH_CODE_COL,c->code,
            DASH_G0,strlen(c->guest_name[0])>0?c->guest_name[0]:"—",
            DASH_G1,strlen(c->guest_name[1])>0?c->guest_name[1]:"—",
            DASH_G2,strlen(c->guest_name[2])>0?c->guest_name[2]:"—",
            DASH_G3,strlen(c->guest_name[3])>0?c->guest_name[3]:"—",-1);
    }
    g_array_free(persons,TRUE);g_array_free(cats,TRUE);
}

static void on_dash_unlock(GtkButton *btn,gpointer data){
    (void)btn;DashState *d=(DashState*)data;
    const char *code=gtk_editable_get_text(GTK_EDITABLE(d->entry_dash_code));
    if(strcmp(code,DASH_CODE)==0){
        gtk_stack_set_visible_child_name(GTK_STACK(d->dash_stack),"content");
        dashboard_refresh(NULL,d);
        gtk_editable_set_text(GTK_EDITABLE(d->entry_dash_code),"");
    } else {
        gtk_label_set_text(GTK_LABEL(d->lbl_dash_lock_status),"⚠  Incorrect access code.");
    }
}
static void on_dash_lock(GtkButton *btn,gpointer data){
    (void)btn;DashState *d=(DashState*)data;
    gtk_stack_set_visible_child_name(GTK_STACK(d->dash_stack),"lock");
}

static GtkWidget *build_dashboard(DashState *d){
    d->dash_stack=gtk_stack_new();

    /* Lock page */
    GtkWidget *lb=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);
    gtk_widget_set_margin_start(lb,80);gtk_widget_set_margin_end(lb,80);
    gtk_widget_set_margin_top(lb,80);gtk_widget_set_margin_bottom(lb,80);
    GtkWidget *lt=gtk_label_new("🔒  Admin Dashboard");
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.5));
    gtk_label_set_attributes(GTK_LABEL(lt),a);pango_attr_list_unref(a);
    gtk_widget_set_halign(lt,GTK_ALIGN_CENTER);gtk_box_append(GTK_BOX(lb),lt);
    GtkWidget *lh=gtk_label_new("The Dashboard is reserved for the Administrator.\nEnter the access code to unlock it.");
    gtk_label_set_justify(GTK_LABEL(lh),GTK_JUSTIFY_CENTER);gtk_box_append(GTK_BOX(lb),lh);
    d->entry_dash_code=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(d->entry_dash_code),"Access code");
    gtk_entry_set_visibility(GTK_ENTRY(d->entry_dash_code),FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(d->entry_dash_code),GTK_INPUT_PURPOSE_PASSWORD);
    gtk_box_append(GTK_BOX(lb),d->entry_dash_code);
    GtkWidget *bu=gtk_button_new_with_label("Unlock Dashboard");gtk_box_append(GTK_BOX(lb),bu);
    d->lbl_dash_lock_status=gtk_label_new("");gtk_box_append(GTK_BOX(lb),d->lbl_dash_lock_status);
    g_signal_connect(bu,"clicked",G_CALLBACK(on_dash_unlock),d);
    g_signal_connect_swapped(d->entry_dash_code,"activate",G_CALLBACK(on_dash_unlock),d);
    gtk_stack_add_named(GTK_STACK(d->dash_stack),lb,"lock");

    /* Content page */
    GtkWidget *cv=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
    gtk_widget_set_margin_start(cv,24);gtk_widget_set_margin_end(cv,24);
    gtk_widget_set_margin_top(cv,20);gtk_widget_set_margin_bottom(cv,20);
    GtkWidget *top=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
    GtkWidget *dtitle=gtk_label_new("Event Overview");
    PangoAttrList *ta=pango_attr_list_new();
    pango_attr_list_insert(ta,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(ta,pango_attr_scale_new(1.2));
    gtk_label_set_attributes(GTK_LABEL(dtitle),ta);pango_attr_list_unref(ta);
    gtk_widget_set_halign(dtitle,GTK_ALIGN_START);gtk_widget_set_hexpand(dtitle,TRUE);
    gtk_box_append(GTK_BOX(top),dtitle);
    GtkWidget *lock_btn=gtk_button_new_with_label("🔒  Lock");
    g_signal_connect(lock_btn,"clicked",G_CALLBACK(on_dash_lock),d);
    gtk_box_append(GTK_BOX(top),lock_btn);
    gtk_box_append(GTK_BOX(cv),top);

    GtkWidget *r1=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,12);gtk_widget_set_hexpand(r1,TRUE);
#define SF(var,txt) { GtkWidget *sf=gtk_frame_new(NULL);gtk_widget_set_hexpand(sf,TRUE);\
    var=gtk_label_new(txt);\
    gtk_widget_set_margin_start(var,12);gtk_widget_set_margin_end(var,12);\
    gtk_widget_set_margin_top(var,12);gtk_widget_set_margin_bottom(var,12);\
    gtk_frame_set_child(GTK_FRAME(sf),var);gtk_box_append(GTK_BOX(r1),sf); }
    SF(d->lbl_total_guests,"— guests")
    SF(d->lbl_total_cats,  "— categories")
    SF(d->lbl_le,          "Side LE: —")
    SF(d->lbl_la,          "Side LA: —")
    SF(d->lbl_parking,     "Parking: —")
#undef SF
    gtk_box_append(GTK_BOX(cv),r1);

    GtkWidget *r2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,12);gtk_widget_set_hexpand(r2,TRUE);
#define SF2(var,txt) { GtkWidget *sf=gtk_frame_new(NULL);gtk_widget_set_hexpand(sf,TRUE);\
    var=gtk_label_new(txt);\
    gtk_widget_set_margin_start(var,12);gtk_widget_set_margin_end(var,12);\
    gtk_widget_set_margin_top(var,12);gtk_widget_set_margin_bottom(var,12);\
    gtk_frame_set_child(GTK_FRAME(sf),var);gtk_box_append(GTK_BOX(r2),sf); }
    SF2(d->lbl_assigned,  "Assigned: —")
    SF2(d->lbl_unassigned,"Unassigned: —")
#undef SF2
    gtk_box_append(GTK_BOX(cv),r2);

    gtk_box_append(GTK_BOX(cv),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *bl=gtk_label_new("Category guest breakdown");
    gtk_widget_set_halign(bl,GTK_ALIGN_START);
    PangoAttrList *ba=pango_attr_list_new();
    pango_attr_list_insert(ba,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(bl),ba);pango_attr_list_unref(ba);
    gtk_box_append(GTK_BOX(cv),bl);
    d->store_cats=gtk_list_store_new(DASH_N,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    d->tree_cats=gtk_tree_view_new_with_model(GTK_TREE_MODEL(d->store_cats));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(d->tree_cats),TRUE);
    const char *cols[DASH_N]={"Category","Guest 1","Guest 2","Guest 3","Guest 4"};
    for(int c=0;c<DASH_N;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(cols[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(d->tree_cats),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),d->tree_cats);
    gtk_box_append(GTK_BOX(cv),scroll);
    d->btn_refresh=gtk_button_new_with_label("🔄  Refresh");
    gtk_box_append(GTK_BOX(cv),d->btn_refresh);
    g_signal_connect(d->btn_refresh,"clicked",G_CALLBACK(dashboard_refresh),d);
    gtk_stack_add_named(GTK_STACK(d->dash_stack),cv,"content");
    gtk_stack_set_visible_child_name(GTK_STACK(d->dash_stack),"lock");
    return d->dash_stack;
}

/* ── Activate ────────────────────────────────── */
static void activate(GtkApplication *app,gpointer user_data){
    (void)user_data;
    GtkWidget *window=gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),"WIGMS — Home");
    gtk_window_set_default_size(GTK_WINDOW(window),820,560);
    gtk_window_set_resizable(GTK_WINDOW(window),TRUE);

    GtkWidget *root=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);

    /* Header */
    GtkWidget *header=gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_widget_set_margin_start(header,32);gtk_widget_set_margin_end(header,32);
    gtk_widget_set_margin_top(header,24);gtk_widget_set_margin_bottom(header,16);
    GtkWidget *lt=gtk_label_new("WIGMS");gtk_widget_set_halign(lt,GTK_ALIGN_START);
    PangoAttrList *ta=pango_attr_list_new();
    pango_attr_list_insert(ta,pango_attr_weight_new(PANGO_WEIGHT_HEAVY));
    pango_attr_list_insert(ta,pango_attr_scale_new(2.0));
    gtk_label_set_attributes(GTK_LABEL(lt),ta);pango_attr_list_unref(ta);
    gtk_box_append(GTK_BOX(header),lt);
    GtkWidget *ls=gtk_label_new("Wedding & Invitation Guest Management System");
    gtk_widget_set_halign(ls,GTK_ALIGN_START);gtk_box_append(GTK_BOX(header),ls);
    gtk_box_append(GTK_BOX(root),header);
    gtk_box_append(GTK_BOX(root),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *nb=gtk_notebook_new();gtk_widget_set_vexpand(nb,TRUE);

    /* Home tab */
    GtkWidget *home=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_widget_set_margin_start(home,16);gtk_widget_set_margin_end(home,16);
    gtk_widget_set_margin_top(home,16);gtk_widget_set_margin_bottom(home,16);
    GtkWidget *lbl_status=gtk_label_new("");gtk_widget_set_halign(lbl_status,GTK_ALIGN_CENTER);
    GtkWidget *cards=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,16);
    gtk_widget_set_margin_bottom(cards,12);

    /* Guest Manager — public */
    gtk_box_append(GTK_BOX(cards),
        build_card_public(
            "👤  Guest Management",
            "Register, update, delete and search guests.\nParking managed automatically.",
            "Open Guest Manager","person_gtk.exe",lbl_status));

    /* Category Manager — admin only (password dialog on click) */
    gtk_box_append(GTK_BOX(cards),
        build_card_protected(
            "🗂  Category Management",
            "Assign registered guests to categories.\nAdmin access required.",
            "Open Category Manager",lbl_status));

    gtk_box_append(GTK_BOX(home),cards);
    gtk_box_append(GTK_BOX(home),lbl_status);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),home,gtk_label_new("🏠  Home"));

    /* Dashboard tab — locked by "pokemon 1" */
    DashState *d=g_new0(DashState,1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_dashboard(d),gtk_label_new("🔒  Dashboard"));

    gtk_box_append(GTK_BOX(root),nb);

    /* Footer */
    gtk_box_append(GTK_BOX(root),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *footer=gtk_label_new("WIGMS v4.0  —  All .exe files must be in the same folder");
    gtk_widget_set_margin_top(footer,8);gtk_widget_set_margin_bottom(footer,8);
    PangoAttrList *fa=pango_attr_list_new();
    pango_attr_list_insert(fa,pango_attr_scale_new(0.85));
    gtk_label_set_attributes(GTK_LABEL(footer),fa);pango_attr_list_unref(fa);
    gtk_box_append(GTK_BOX(root),footer);

    gtk_window_set_child(GTK_WINDOW(window),root);
    gtk_widget_set_visible(window,TRUE);
}

int main(int argc,char **argv){
    GtkApplication *app=gtk_application_new("cm.wigms.home",G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
    int s=g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);return s;
}
