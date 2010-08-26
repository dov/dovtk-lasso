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
int start_x, start_y, end_x, end_y;

void draw_caliper(cairo_t *cr,
                  gboolean do_mask,
                  double x0, double y0,
                  double x1, double y1)
{
    int margin = 0;
    if (do_mask)
        margin = 5;

    double angle = atan2(y1-y0,x1-x0);
    cairo_translate(cr,
                    0.5 * (x0+x1),
                    0.5 * (y0+y1));
    cairo_rotate(cr, angle);
    double dist = sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));

    if (!do_mask)
        cairo_set_source_rgba(cr, 10*0x4d/255.0,1.0*0xaa/255.0,0,0.5);
    cairo_rectangle(cr, -dist/2-margin, -20-margin,
                    dist+2*margin, 20+2*margin);
    cairo_fill(cr);

    if (!do_mask)
        cairo_set_source_rgb(cr, 0x50/255.0,0x2d/255.0,0x16/255.0);

    double calip_height = 50;
    cairo_move_to(cr, -dist/2+margin,calip_height/2+margin); 
    double dy = -(calip_height+3*margin)/3;
    cairo_rel_curve_to(cr,
                       -15-2*margin,dy,
                       -15-2*margin,2.5*dy,
                       -15-2*margin,3*dy);
    cairo_line_to(cr, -dist/2+margin,-calip_height/2-margin); 
                  
    cairo_close_path(cr);
        
    cairo_move_to(cr, dist/2-margin,calip_height/2+margin); 
    cairo_rel_curve_to(cr,
                       15+2*margin,dy,
                       15+2*margin,2.5*dy,
                       15+2*margin,3*dy);
    cairo_line_to(cr, dist/2-margin,-calip_height/2-margin); 
                  
    cairo_close_path(cr);

    if (do_mask) {
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 5);
        cairo_stroke(cr);
    }
    else 
        cairo_fill(cr);
    
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
    double radius_max = MIN (widget->allocation.width / 2,
                             widget->allocation.height / 2)-5;
    for (i=0; i<10; i++) {
        double radius;
        radius =  50 * i;

        cairo_set_source_rgb(cr, 0,0,0);
        cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
        
   return FALSE;
}

/** 
 * Draw  whatever overlay you want on the image. If the do_mask
 * is on, then you should paint in black and with a pen that
 * is thicker than the drawing. 
 */
void my_lasso_draw(cairo_t *cr,
                   gboolean do_mask,
                   // output
                   DovtkLassoRectangleList **rect_list)
{
    int min_x = MIN(start_x, end_x);
    int min_y = MIN(start_y, end_y);

    if (!do_mask) {
        cairo_set_source_rgb(cr, 1,0,0);
        cairo_set_line_width(cr,1);
    }
    else
        cairo_set_line_width(cr, 5);

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // Draw a rectangle
    //    cairo_rectangle(cr, min_x, min_y, abs(end_x-start_x), abs(end_y-start_y));
    draw_caliper(cr,
                 do_mask,
                 start_x, start_y,
                 end_x, end_y);

    cairo_stroke(cr);
}

int cb_button_press(GtkWidget      *widget,
                    GdkEventButton *event,
                    gpointer        user_data)
{
    lasso = dovtk_lasso_create(widget,
                               &my_lasso_draw,
                               TRUE,
                               user_data);
    start_x = event->x;
    start_y = event->y;
    end_x = start_x;
    end_y = start_y;
    dovtk_lasso_update(lasso);

    return FALSE;
}

int cb_button_release(GtkWidget      *widget,
                      GdkEventButton *event,
                      gpointer        user_data)
{
    dovtk_lasso_destroy(lasso);
    lasso = NULL;
    return FALSE;
}

int cb_motion_notify(GtkWidget      *widget,
                     GdkEventMotion *event,
                     gpointer        user_data)
{
    //    printf("button motion\n");
    end_x = event->x;
    end_y = event->y;

    dovtk_lasso_update(lasso);

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
    
    gtk_widget_show_all(w_top);
    gtk_main();

    return 0;
}
