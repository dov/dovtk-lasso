/** 
 * dovtk-lasso.c
 *
 * A solution for drawing overlays on a gtk widget.
 * 
 * This code is relased under the LGPL v2.0.
 *
 * Copyright Dov Grobgeld <dov.grobgeld@gmail.com> 2010
 * 
 */
#include "dovtk-lasso.h"

typedef struct {
    DovtkLasso parent;
    gulong expose_handler_id;
    GtkWidget *widget;
    DovtkLassoDrawing drawing_cb;
    gboolean do_calc_expose_from_cairo;
    DovtkLassoRectangleList *old_rect_list;
    gpointer user_data;
} DovtkLassoPrivate ;

static int lasso_cb_expose(GtkWidget      *widget,
                           GdkEventExpose *event,
                           gpointer        user_data);

DovtkLasso *dovtk_lasso_create(GtkWidget *widget,
                               DovtkLassoDrawing drawing_cb,
                               gboolean do_calc_expose_from_cairo,
                               gpointer user_data)
{
    DovtkLassoPrivate *selfp = g_new0(DovtkLassoPrivate, 1);
    
    // This binding doesn't work if the default expose handler
    // returns TRUE!
    selfp->expose_handler_id
        = g_signal_connect_after(widget,
                                 "expose-event",
                                 G_CALLBACK(lasso_cb_expose),
                                 selfp);
    selfp->widget = widget;
    selfp->drawing_cb = drawing_cb;
    selfp->do_calc_expose_from_cairo = do_calc_expose_from_cairo;
    selfp->user_data = user_data;
    // Create an empty list so that we can free it
    selfp->old_rect_list = dovtk_lasso_rectangle_list_new(0);
    return (DovtkLasso*)selfp;
}

void dovtk_lasso_destroy(DovtkLasso *lasso)
{
    DovtkLassoPrivate *selfp = (DovtkLassoPrivate*)lasso;
    g_signal_handler_disconnect(selfp->widget,
                                selfp->expose_handler_id);
    // This gets rid of the overlay. Is this always needed?
    dovtk_lasso_update(lasso);
    
    dovtk_lasso_rectangle_list_destroy(selfp->old_rect_list);

    g_free(lasso);
}

static int lasso_cb_expose(GtkWidget      *widget,
                           GdkEventExpose *event,
                           gpointer        user_data)
{
    DovtkLassoPrivate *selfp = (DovtkLassoPrivate*)user_data;
    //    printf("dovtk-lasso.c: expose\n");

#if 0
    g_signal_handler_block(widget, selfp->expose_handler_id);
    int retval;
    g_signal_emit_by_name (widget, "expose-event", event, &retval);
    g_signal_handler_unblock(widget, selfp->expose_handler_id);
#endif

    cairo_t *cr;
    cr = gdk_cairo_create(widget->window);
    cairo_rectangle(cr, event->area.x, event->area.y,
                    event->area.width, event->area.height);
    cairo_clip(cr);

    DovtkLassoRectangleList *rect_list = NULL;
    selfp->drawing_cb(cr, FALSE, selfp->user_data, &rect_list);

    cairo_destroy(cr);

    return TRUE;
}

int a8_idx=0;

void dovtk_lasso_update(DovtkLasso *lasso)
{
    DovtkLassoPrivate *selfp = (DovtkLassoPrivate*)lasso;

    // Call drawing_cb to and use it to generate the rectangle list
    DovtkLassoRectangleList *rect_list = NULL;
    int scale_factor = 32;
    int low_res_width = (selfp->widget->allocation.width+scale_factor-1) / scale_factor;
    int low_res_height = (selfp->widget->allocation.height+scale_factor-1) / scale_factor;
    
    int i;

    // This should be created in the creation of DovtkLasso
    cairo_t *cr = NULL;
    cairo_surface_t *surf = NULL;

    if (selfp->do_calc_expose_from_cairo) {
        surf=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                        low_res_width,
                                        low_res_height);
        cr = cairo_create(surf);
        cairo_set_source_rgba(cr,0,0,0,0);
        cairo_rectangle(cr, 0,0,low_res_height,low_res_width);
        cairo_fill(cr);
        cairo_set_source_rgba(cr,0,0,0,1);
    }
    cairo_scale(cr,1.0/scale_factor,1.0/scale_factor);
    selfp->drawing_cb(cr, TRUE, selfp->user_data, &rect_list);
#if 0
    char filename[64];
    sprintf(filename, "/tmp/a8-%04d.png", a8_idx++);
    cairo_surface_write_to_png(surf, filename);
#endif

    // TBD - Turn surf into a list of rectangles
    if (selfp->do_calc_expose_from_cairo) {
        int row_idx, col_idx;

        // Allocate a lot of space
        rect_list = dovtk_lasso_rectangle_list_new(low_res_width*low_res_height);

        guint8 *buf = cairo_image_surface_get_data(surf);
        int rect_idx = 0;
        int row_stride = cairo_image_surface_get_stride(surf);
        for (row_idx=0; row_idx<low_res_height; row_idx++) {
            for (col_idx=0; col_idx<low_res_width; col_idx++) {
                if (*(buf + row_stride * row_idx + col_idx * 4+3) > 0) {
                    cairo_rectangle_t *rect = &rect_list->rectangles[rect_idx++];
                    rect->x = col_idx*scale_factor;
                    rect->y = row_idx*scale_factor;
                    rect->width = scale_factor;
                    rect->height = scale_factor;
                }
            }
        }
        rect_list->num_rectangles = rect_idx;
        
        cairo_destroy(cr);
        cairo_surface_destroy(surf);
    }
    //    printf("num_rectangles = %d\n", rect_list->num_rectangles);

    // Build a list of expose rectangles from the old and the new lists.
    // Better done as a linked list.
    DovtkLassoRectangleList *expose_rect_list
        = dovtk_lasso_rectangle_list_new(selfp->old_rect_list->num_rectangles
                                         + rect_list->num_rectangles);
    int num_old_rects = selfp->old_rect_list->num_rectangles;
    for (i=0; i<num_old_rects; i++) 
        expose_rect_list->rectangles[i] = selfp->old_rect_list->rectangles[i];
    for (i=0; i<rect_list->num_rectangles; i++)
        expose_rect_list->rectangles[num_old_rects + i] = rect_list->rectangles[i];

    // Expose the old and the new list of rectangles!
    for (i=0; i<expose_rect_list->num_rectangles; i++) {
        // Shortcut
        cairo_rectangle_t *lasso_rect = &expose_rect_list->rectangles[i];
        
        GdkRectangle rect;
        rect.x = lasso_rect->x;
        rect.y = lasso_rect->y;
        rect.width = lasso_rect->width;
        rect.height = lasso_rect->height;
#if 0
        printf("Invalidate region (%d,%d,%d,%d).\n",
               rect.x,rect.y,rect.width,rect.height);
#endif
        gdk_window_invalidate_rect(selfp->widget->window,
                                   &rect,
                                   TRUE);
    }
    dovtk_lasso_rectangle_list_destroy(expose_rect_list);

    dovtk_lasso_rectangle_list_destroy(selfp->old_rect_list);
    selfp->old_rect_list = rect_list;
}

DovtkLassoRectangleList *dovtk_lasso_rectangle_list_new(int num_rectangles)
{
    DovtkLassoRectangleList *rectangle_list = g_new0(DovtkLassoRectangleList, 1);
    rectangle_list->num_rectangles = num_rectangles;
    rectangle_list->rectangles = g_new0(cairo_rectangle_t, num_rectangles);
    return rectangle_list;
}

void dovtk_lasso_rectangle_list_destroy(DovtkLassoRectangleList *rectangle_list)
{
    g_free(rectangle_list->rectangles);
    g_free(rectangle_list);
}
