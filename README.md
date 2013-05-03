# Description

dovtk-lasso is a general framework for doing interactive overlay selections annotations in gtk.

See the file test-dovtk-lasso.c for an example of how to use. It 
draws a caliper like measurement device.

# Usage 

To use dovtk-lasso a callback function with the following signature has to be created:

    typedef void (*DovtkLassoDrawing)(cairo_t *cr,
                                      DovtkLassoContext Context,
                                      gpointer user_data);
                                      
                                      
The variable `Context` defines one of the three modes that this function is used for:

* `DOVTK_LASSO_CONTEXT_PAINT` - The function will be used for drawing the overlay graphics.
* `DOVTK_LASSO_CONTEXT_MASK` - The function will be used for creation of a low resolution mask. When in this mode the color shouldn't be changed, but has already been set up by gtk lasso.
* `DOVTK_LASSO_CONTEXT_LABEL` - Used to drawing label areas that may be used for picking and moving. E.g. a caliper may have a left, a right and a center area.

To start using a lasso, first create a lasso instance on any gtk widget and pass the lasso draw function as a variable:

    lasso = dovtk_lasso_create(w_draw,
                               &my_lasso_draw,
                               NULL);

Create button-press, button-release, and motion_event callbacks an use these to create and interact with an existing overlay.

In the button-press callback the label of the pixel may be queried to get information of what area should be moved:

    pixel_label  = dovtk_lasso_get_label_for_pixel(lasso, event->x, event->y);
    
If the graphics of the overlay should be changed e.g. by the button_press or the motion_event callbacks, dovtk_lasso should be informed by doing:

   dovtk_lasso_update(lasso);

# Reference

This is work in progress based on the following email that I wrote:

gtk-app-devel-list@gnome.org
date	Tue, Aug 10, 2010 at 23:14
subject	Re: porting gdk -> cairo


The following more complex solution should take care of flickering:

Preparation:

    * Create an expose handle that is called after the default expose handle which uses cairo to draw a rectangle in coordinates determined by structure R (see below). Note that this expose handle will be called up to eight times every time a motion notify event occurs.
    * Create a fifo buffer containing up to eight CairoRectangles to be exposed.

During motion:

    * Define the new rectangle to draw by button-press and motion-notify events and store it in the structure R available by the expose handle.
    * Push four areas corresponding to the four edges of the rectangles to the fifo buffer.
    * Expose the eight areas in the fifo buffer.
    * Pop off the first four areas of the fifo buffer.

This solution relies on the underlying expose handle. If that expose handle is slow (e.g. complex vector graphics), the solution may be made faster by initiating the process (e.g. at button-press) by copying the entire widget window to an off screen pixmap, and then block the underlying expose handle.
