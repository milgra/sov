#ifndef ku_renderer_software_h
#define ku_renderer_software_h

#include "ku_bitmap.c"
#include "ku_rect.c"
#include "mt_vector.c"

void ku_renderer_software_render(mt_vector_t* views, ku_bitmap_t* bitmap, ku_rect_t dirty);
void ku_renderer_soft_screenshot(ku_bitmap_t* bitmap, char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_png.c"
#include "ku_view.c"
#include "mt_time.c"

void ku_renderer_software_render(mt_vector_t* views, ku_bitmap_t* bitmap, ku_rect_t dirty)
{
    /* cut out dirty rect */

    /* ku_bitmap_cut(&mmfm.window->bitmap, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h); */

    /* draw dirty rect for debugging */

    /* ku_bitmap_blend_rect(bitmap, (int) dirty.x, (int) dirty.y, (int) dirty.w, (int) dirty.h, 0x55FF0000); */
    /* ku_wayland_draw_window(mmfm.window, 0, 0, mmfm.window->width, mmfm.window->height); */

    /* we need to keep nested masks in mind */

    static ku_rect_t masks[32] = {0};
    static int       maski     = 0;

    masks[0] = dirty;

    /* draw views into bitmap */

    for (size_t i = 0; i < views->length; i++)
    {
	ku_view_t* view = views->data[i];

	if (view->style.masked)
	{
	    dirty = ku_rect_is(masks[maski], view->frame.global);
	    maski++;
	    masks[maski] = dirty;
	    /* printf("%s masked, dirty %f %f %f %f\n", view->id, dirty.x, dirty.y, dirty.w, dirty.h); */
	}

	if (view->texture.bitmap)
	{
	    ku_rect_t rect = view->frame.global;

	    bmr_t dstmsk = ku_bitmap_is(
		(bmr_t){(int) dirty.x, (int) dirty.y, (int) (dirty.x + dirty.w), (int) (dirty.y + dirty.h)},
		(bmr_t){0, 0, bitmap->w, bitmap->h});

	    bmr_t srcmsk = {0, 0, view->texture.bitmap->w, view->texture.bitmap->h};

	    /* if there is a region to draw, modify source mask */

	    if (view->frame.region.w > -1 && view->frame.region.h > -1)
	    {
		srcmsk.x += view->frame.region.x;
		srcmsk.y += view->frame.region.y;
		srcmsk.z = srcmsk.x + view->frame.region.w;
		srcmsk.w = srcmsk.y + view->frame.region.h;
	    }

	    /* draw with shadow blur outlets in mind */

	    if (view->texture.transparent == 0 || i == 0)
	    {
		ku_bitmap_insert(
		    bitmap,
		    dstmsk,
		    view->texture.bitmap,
		    srcmsk,
		    rect.x - view->style.shadow_blur,
		    rect.y - view->style.shadow_blur);
	    }
	    else
	    {
		if (view->texture.alpha == 1.0)
		{
		    ku_bitmap_blend(
			bitmap,
			dstmsk,
			view->texture.bitmap,
			srcmsk,
			rect.x - view->style.shadow_blur,
			rect.y - view->style.shadow_blur);
		}
		else
		{
		    ku_bitmap_blend_with_alpha(
			bitmap,
			dstmsk,
			view->texture.bitmap,
			srcmsk,
			rect.x - view->style.shadow_blur,
			rect.y - view->style.shadow_blur,
			(255 - (int) (view->texture.alpha * 255.0)));
		}
	    }
	}

	if (view->style.unmask)
	{
	    maski--;
	    dirty = masks[maski];
	}
    }
}

void ku_renderer_soft_screenshot(ku_bitmap_t* bitmap, char* path)
{
    ku_png_write(path, bitmap);
}

#endif
