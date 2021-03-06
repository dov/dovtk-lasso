/** 
 * test-dovtk-lasso.h
 *
 * A solution for drawing overlays on a gtk widget.
 * 
 * This example is in the public domain.
 *
 */
#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include "dovtk-lasso.h"

DovtkLasso *lasso = NULL;
int start_x=-1, start_y=-1, end_x=-1, end_y=-1;
int picking_start_x=-1, picking_start_y=-1;
gboolean is_defining_lasso = FALSE;
int picking = -1;

void draw_caliper(cairo_t *cr,
                  DovtkLassoContext context,
                  double x0, double y0,
                  double x1, double y1)
{
    int margin = 0;
    if (context != DOVTK_LASSO_CONTEXT_PAINT)
        margin = 5;

    double angle = atan2(y1-y0,x1-x0);
    cairo_translate(cr,
                    0.5 * (x0+x1),
                    0.5 * (y0+y1));
    cairo_rotate(cr, angle);
    double dist = sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));

    if (context == DOVTK_LASSO_CONTEXT_PAINT)
        cairo_set_source_rgba(cr, 10*0x4d/255.0,1.0*0xaa/255.0,0,0.5);
    else if (context == DOVTK_LASSO_CONTEXT_LABEL)
        dovtk_lasso_set_color_label(lasso, cr, 1);
    
    cairo_rectangle(cr, -dist/2-margin, -20-margin,
                    dist+2*margin, 20+2*margin);
    cairo_fill(cr);

    if (context == DOVTK_LASSO_CONTEXT_PAINT)
        cairo_set_source_rgb(cr, 0x50/255.0,0x2d/255.0,0x16/255.0);
    else if (context == DOVTK_LASSO_CONTEXT_LABEL)
        dovtk_lasso_set_color_label(lasso, cr, 2);

    double calip_height = 50;
    cairo_move_to(cr, -dist/2+margin,calip_height/2+margin); 
    double dy = -(calip_height+3*margin)/3;
    cairo_rel_curve_to(cr,
                       -15-2*margin,dy,
                       -15-2*margin,2.5*dy,
                       -15-2*margin,3*dy);
    cairo_line_to(cr, -dist/2+margin,-calip_height/2-margin); 
                  
    cairo_close_path(cr);
        
    if (context == DOVTK_LASSO_CONTEXT_LABEL) {
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 5);
        cairo_stroke(cr);
        dovtk_lasso_set_color_label(lasso, cr, 3);
    }

    cairo_move_to(cr, dist/2-margin,calip_height/2+margin); 
    cairo_rel_curve_to(cr,
                       15+2*margin,dy,
                       15+2*margin,2.5*dy,
                       15+2*margin,3*dy);
    cairo_line_to(cr, dist/2-margin,-calip_height/2-margin); 
                  
    cairo_close_path(cr);

    if (context == DOVTK_LASSO_CONTEXT_PAINT) 
        cairo_fill(cr);
    else {
        cairo_set_line_width(cr, 5);
        cairo_fill_preserve(cr);
        cairo_stroke(cr);
    }
    
    if (context != DOVTK_LASSO_CONTEXT_LABEL) {
        // Draw the distance in the middle
        PangoFontDescription *font_descr = pango_font_description_from_string("Sans 15");
        PangoLayout *pango_layout = pango_cairo_create_layout(cr);
        pango_layout_set_font_description(pango_layout, font_descr);
        gchar *dist_str = g_strdup_printf("%.1f", dist);
        pango_layout_set_text(pango_layout, dist_str, -1);
        int layout_width, layout_height;
        pango_layout_get_size(pango_layout, &layout_width, &layout_height);
    
        cairo_move_to(cr, -0.5*layout_width/PANGO_SCALE,-20);
        pango_cairo_show_layout(cr, pango_layout);
        g_object_unref(pango_layout);
        pango_font_description_free(font_descr);
      }
}

int cb_expose(GtkWidget      *widget,
              GdkEventExpose *event,
              gpointer        user_data)
{
    cairo_t *cr;
    cr = gdk_cairo_create(widget->window);
    cairo_rectangle(cr, event->area.x, event->area.y,
                    event->area.width, event->area.height);
    cairo_clip(cr);

    // Just draw anything in the widget
    double x, y;
    x = widget->allocation.x + widget->allocation.width / 2;
    y = widget->allocation.y + widget->allocation.height / 2;

    int i;

    for (i=0; i<10; i++) {
        double radius;
        radius =  50 * i;

        cairo_set_source_rgb(cr, 0,0,0);
        cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
        cairo_stroke(cr);
    }

    if (start_x >= 0)
        draw_caliper(cr, DOVTK_LASSO_CONTEXT_PAINT, start_x, start_y, end_x, end_y);
                     
    cairo_destroy(cr);
        
   return FALSE;
}

/** 
 * Draw  whatever overlay you want on the image. If the do_mask
 * is on, then you should paint in black and with a pen that
 * is thicker than the drawing. 
 */
void my_lasso_draw(cairo_t *cr,
                   DovtkLassoContext context,
                   gpointer user_data)
{
    // Draw a rectangle
    //    cairo_rectangle(cr, min_x, min_y, abs(end_x-start_x), abs(end_y-start_y));
    if (start_x>0)
        draw_caliper(cr,
                     context,
                     start_x, start_y,
                     end_x, end_y);

    cairo_stroke(cr);
}

int cb_button_press(GtkWidget      *widget,
                    GdkEventButton *event,
                    gpointer        user_data)
{
    picking = dovtk_lasso_get_label_for_pixel(lasso, event->x, event->y);
    if (picking) {
        picking_start_x = event->x;
        picking_start_y = event->y;
    }
    else {
        start_x = event->x;
        start_y = event->y;
        end_x = start_x;
        end_y = start_y;
        is_defining_lasso= TRUE;
        dovtk_lasso_update(lasso);
    }

    return FALSE;
}

int cb_button_release(GtkWidget      *widget,
                      GdkEventButton *event,
                      gpointer        user_data)
{
    is_defining_lasso= FALSE;
    picking = 0;
    return FALSE;
}

int cb_motion_notify(GtkWidget      *widget,
                     GdkEventMotion *event,
                     gpointer        user_data)
{
    if (picking) {
        int dx = event->x - picking_start_x;
        int dy = event->y - picking_start_y;
        if (picking == 1 || picking==2) {
            start_x += dx;
            start_y += dy;
        }
        if (picking == 1 || picking==3) {
            end_x += dx;
            end_y += dy;
        }
        picking_start_x = event->x;
        picking_start_y = event->y;
        dovtk_lasso_update(lasso);
    }
    else if (is_defining_lasso) {
        //    printf("button motion\n");
        end_x = event->x;
        end_y = event->y;

        dovtk_lasso_update(lasso);
    }
        

    return FALSE;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    GtkWidget *w_top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(w_top), "delete-event",
                     G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *w_draw = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(w_top),
                      w_draw);
    gtk_widget_set_size_request(w_draw, 500,500);
    g_signal_connect(G_OBJECT(w_draw), "expose-event",
                     G_CALLBACK(cb_expose), NULL);
                     
    // TBD - set up events for lasso
    gtk_widget_add_events(w_draw,
                          GDK_BUTTON_MOTION_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(w_draw), "button-press-event",
                     G_CALLBACK(cb_button_press), NULL);
    g_signal_connect(G_OBJECT(w_draw), "button-release-event",
                     G_CALLBACK(cb_button_release), NULL);
    g_signal_connect(G_OBJECT(w_draw), "motion-notify-event",
                     G_CALLBACK(cb_motion_notify), NULL);
    
    lasso = dovtk_lasso_create(w_draw,
                               &my_lasso_draw,
                               NULL);

    gtk_widget_show_all(w_top);
    gtk_main();

    return 0;
}

