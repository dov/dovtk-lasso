/** 
 * dovtk-lasso.h
 *
 * A solution for drawing overlays on a gtk widget.
 * 
 * This code is relased under the LGPL v2.0.
 *
 * Copyright Dov Grobgeld <dov.grobgeld@gmail.com> 2010
 * 
 */
#ifndef DOVTK_H
#define DOVTK_H

#include <gtk/gtk.h>

typedef struct {
} DovtkLasso;

typedef struct {
    int num_rectangles;
    cairo_rectangle_t *rectangles;
} DovtkLassoRectangleList;

typedef void (*DovtkLassoDrawing)(cairo_t *cr,
                                  gboolean do_mask,
                                  // output
                                  DovtkLassoRectangleList **rect_list);

DovtkLasso *dovtk_lasso_create(GtkWidget *widget,
                               DovtkLassoDrawing drawing_cb,
                               gboolean do_calc_expose_from_cairo);

/** 
 * Called when the coordinates of the lasso were changed.
 * 
 * @param lasso 
 */
void dovtk_lasso_update(DovtkLasso *lasso);

void dovtk_lasso_destroy(DovtkLasso *lasso);

DovtkLassoRectangleList *dovtk_lasso_rectangle_list_new(int num_rectangles);
void dovtk_lasso_rectangle_list_destroy(DovtkLassoRectangleList *rectangcle_list);
#endif /* DOVTK */
