#ifndef ku_gl_h
#define ku_gl_h

#include "ku_bitmap.c"
#include "mt_vector.c"
/* #include <GL/glew.h> */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>

void ku_gl_init(int max_dev_width, int max_dev_height);
void ku_gl_destroy();
void ku_gl_render(ku_bitmap_t* bitmap);
void ku_gl_render_quad(ku_bitmap_t* bitmap, uint32_t index, bmr_t mask);
void ku_gl_add_textures(mt_vector_t* views, int force);
void ku_gl_add_vertexes(mt_vector_t* views);
void ku_gl_clear_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h);
void ku_gl_save_framebuffer(ku_bitmap_t* bitmap);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_gl_atlas.c"
#include "ku_gl_floatbuffer.c"
#include "ku_gl_shader.c"
#include "ku_view.c"
#include "mt_log.c"
#include "mt_math_3d.c"

typedef struct _glbuf_t
{
    GLuint vbo;
    GLuint vao;
} glbuf_t;

typedef struct _gltex_t
{
    GLuint index;
    GLuint tx;
    GLuint fb;
    GLuint w;
    GLuint h;
} gltex_t;

struct ku_gl_t
{
    glsha_t shader;
    glbuf_t buffer;
    gltex_t texture;

    ku_gl_atlas_t*    atlas;
    ku_floatbuffer_t* floatbuffer;
    int               texturesize;
} kugl = {0};

glsha_t ku_gl_create_texture_shader()
{
    char* vsh =
	"#version 100\n"
	"attribute vec3 position;"
	"attribute vec4 texcoord;"
	"uniform mat4 projection;"
	"varying highp vec4 vUv;"
	"void main ( )"
	"{"
	"    gl_Position = projection * vec4(position,1.0);"
	"    vUv = texcoord;"
	"}";

    char* fsh =
	"#version 100\n"
	"uniform sampler2D sampler[10];"
	"varying highp vec4 vUv;"
	"void main( )"
	"{"
	" highp float alpha = vUv.w;"
	" highp vec4 col = texture2D(sampler[0], vUv.xy);"
	" if (alpha < 1.0) col.w *= alpha;"
	" gl_FragColor = col;"
	"}";

    glsha_t sha = ku_gl_shader_create(
	vsh,
	fsh,
	2,
	((const char*[]){"position", "texcoord"}),
	11,
	((const char*[]){"projection", "sampler[0]", "sampler[1]", "sampler[2]", "sampler[3]", "sampler[4]", "sampler[5]", "sampler[6]", "sampler[7]", "sampler[8]", "sampler[9]"}));

    glUseProgram(sha.name);
    glUniform1i(sha.uni_loc[1], 0);

    return sha;
}

/* creates vertex buffer and vertex array object */

glbuf_t ku_gl_create_vertex_buffer()
{
    glbuf_t vb = {0};

    glGenBuffers(1, &vb.vbo); // DEL 0
    glBindBuffer(GL_ARRAY_BUFFER, vb.vbo);
    glGenVertexArrays(1, &vb.vao); // DEL 1
    glBindVertexArray(vb.vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (const GLvoid*) 8);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vb;
}

/* deletes vertex buffer */

void ku_gl_delete_vertex_buffer(glbuf_t buf)
{
    glDeleteBuffers(1, &buf.vbo);
    glDeleteVertexArrays(1, &buf.vao);
}

/* create texture */

gltex_t ku_gl_create_texture(int index, uint32_t w, uint32_t h)
{
    gltex_t tex = {0};

    tex.index = index;
    tex.w     = w;
    tex.h     = h;

    glGenTextures(1, &tex.tx); // DEL 0

    glActiveTexture(GL_TEXTURE0 + tex.index);
    glBindTexture(GL_TEXTURE_2D, tex.tx);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* generate framebuffer for texture */
    /* glGenFramebuffers(1, &tex.fb); // DEL 1 */

    /* glBindFramebuffer(GL_FRAMEBUFFER, tex.fb); */
    /* glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.tx, 0); */
    /* glClearColor(1.0, 0.0, 0.0, 1.0); */
    /* glClear(GL_COLOR_BUFFER_BIT); */
    /* glBindFramebuffer(GL_FRAMEBUFFER, 0); */

    return tex;
}

/* deltes texture */

void ku_gl_delete_texture(gltex_t tex)
{
    glDeleteTextures(1, &tex.tx);     // DEL 0
    glDeleteFramebuffers(1, &tex.fb); // DEL 1
}

/* TODO move gl initialization code here from wayland connector */

void ku_gl_init(int max_dev_width, int max_dev_height)
{
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
	printf("ERROR 5 %s\n", glewGetErrorString(err));
    }

    kugl.shader = ku_gl_create_texture_shader();
    kugl.buffer = ku_gl_create_vertex_buffer();

    kugl.floatbuffer = ku_floatbuffer_new();

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    /* calculate needed texture size, should be four times bigger than the highest resolution display */

    int size = max_dev_width > max_dev_height ? max_dev_width : max_dev_height;
    size *= 2;
    int binsize = 2;
    while (binsize < size) binsize *= 2;

    /* get max texture size */

    int value;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);

    /* don't let it be bigger than max size */

    if (binsize > value) binsize = value;

    mt_log_info("Texture size will be %i", binsize);

    kugl.texturesize = binsize;

    kugl.atlas   = ku_gl_atlas_new(kugl.texturesize, kugl.texturesize);
    kugl.texture = ku_gl_create_texture(0, kugl.texturesize, kugl.texturesize);

    glClearColor(0.0, 0.0, 0.0, 0.6);
}

/* destroy ku gl */

void ku_gl_destroy()
{
    REL(kugl.floatbuffer);
    REL(kugl.atlas);

    ku_gl_delete_texture(kugl.texture);
    ku_gl_delete_vertex_buffer(kugl.buffer);

    glDeleteProgram(kugl.shader.name);
}

/* upload textures */

void ku_gl_add_textures(mt_vector_t* views, int force)
{
    /* reset atlas */
    if (force)
    {
	if (kugl.texturesize != kugl.atlas->width)
	{
	    /* resize texture if needed */

	    ku_gl_delete_texture(kugl.texture);
	    kugl.texture = ku_gl_create_texture(0, kugl.texturesize, kugl.texturesize);
	}

	/* reset atlas */

	if (kugl.atlas) REL(kugl.atlas);
	kugl.atlas = ku_gl_atlas_new(kugl.texturesize, kugl.texturesize);
    }

    glBindTexture(GL_TEXTURE_2D, kugl.texture.tx);
    glActiveTexture(GL_TEXTURE0 + kugl.texture.index);

    int reset = 0;

    /* add texture to atlas */
    for (int index = 0; index < views->length; index++)
    {
	ku_view_t* view = views->data[index];

	/* does it fit into the texture atlas? */
	if (view->texture.bitmap)
	{
	    if (view->texture.bitmap->w < kugl.texturesize &&
		view->texture.bitmap->h < kugl.texturesize)
	    {
		/* do we have to upload it? */
		if (view->texture.uploaded == 0 || force)
		{
		    ku_gl_atlas_coords_t coords = ku_gl_atlas_get(kugl.atlas, view->id);

		    /* did it's size change */
		    if (view->texture.bitmap->w != coords.w || view->texture.bitmap->h != coords.h || force)
		    {
			int success = ku_gl_atlas_put(kugl.atlas, view->id, view->texture.bitmap->w, view->texture.bitmap->h);

			if (success < 0)
			{
			    /* force reset */

			    mt_log_debug("Texture Atlas Reset\n");
			    if (force == 0) reset = 1;
			    break;
			}

			coords = ku_gl_atlas_get(kugl.atlas, view->id);
		    }

		    /* upload texture */

		    glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			coords.x,
			coords.y,
			coords.w,
			coords.h,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			view->texture.bitmap->data);

		    view->texture.uploaded = 1;
		}
	    }
	    else
	    {
		if (force == 0)
		{
		    /* increase texture and atlas size to fit texture */

		    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &kugl.texturesize);

		    mt_log_info("Texture size will be %i", kugl.texturesize);

		    reset = 1;
		}

		/* TODO : auto test texture resize */
	    }
	}
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    /* in case of reset do the whole thing once again with forced reset*/

    if (reset == 1) ku_gl_add_textures(views, 1);
}

/* upload vertexes */

void ku_gl_add_vertexes(mt_vector_t* views)
{
    ku_floatbuffer_reset(kugl.floatbuffer);

    /* add vertexes to buffer */
    for (int index = 0; index < views->length; index++)
    {
	ku_view_t* view = views->data[index];

	ku_rect_t            rect = view->frame.global;
	ku_gl_atlas_coords_t text = ku_gl_atlas_get(kugl.atlas, view->id);

	/* expand with blur thickness */

	if (view->style.shadow_blur > 0)
	{
	    rect.x -= view->style.shadow_blur;
	    rect.y -= view->style.shadow_blur;
	    rect.w += 2 * view->style.shadow_blur;
	    rect.h += 2 * view->style.shadow_blur;
	}

	/* use full texture if needed */

	if (view->texture.full)
	{
	    text.ltx = 0.0;
	    text.lty = 0.0;
	    text.rbx = 1.0;
	    text.rby = 1.0;
	}

	float data[36];

	data[0] = (int) rect.x;
	data[1] = (int) rect.y;

	data[6] = (int) (rect.x + rect.w);
	data[7] = (int) (rect.y + rect.h);

	data[12] = (int) rect.x;
	data[13] = (int) (rect.y + rect.h);

	data[18] = (int) (rect.x + rect.w);
	data[19] = (int) rect.y;

	data[24] = (int) rect.x;
	data[25] = (int) rect.y;

	data[30] = (int) (rect.x + rect.w);
	data[31] = (int) (rect.y + rect.h);

	data[2] = text.ltx;
	data[3] = text.lty;

	data[8] = text.rbx;
	data[9] = text.rby;

	data[14] = text.ltx;
	data[15] = text.rby;

	data[20] = text.rbx;
	data[21] = text.lty;

	data[26] = text.ltx;
	data[27] = text.lty;

	data[32] = text.rbx;
	data[33] = text.rby;

	data[4]  = (float) 0;
	data[10] = (float) 0;
	data[16] = (float) 0;
	data[22] = (float) 0;
	data[28] = (float) 0;
	data[34] = (float) 0;

	data[5]  = view->texture.alpha;
	data[11] = view->texture.alpha;
	data[17] = view->texture.alpha;
	data[23] = view->texture.alpha;
	data[29] = view->texture.alpha;
	data[35] = view->texture.alpha;

	ku_floatbuffer_add(kugl.floatbuffer, data, 36);
    }

    glBindBuffer(GL_ARRAY_BUFFER, kugl.buffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * kugl.floatbuffer->pos, kugl.floatbuffer->data, GL_DYNAMIC_DRAW);
}

/* render all vertexes */

void ku_gl_render(ku_bitmap_t* bitmap)
{
    /* glBindFramebuffer(GL_FRAMEBUFFER, tex_frame.fb); */
    glBindVertexArray(kugl.buffer.vao);

    glActiveTexture(GL_TEXTURE0 + kugl.texture.index);
    glBindTexture(GL_TEXTURE_2D, kugl.texture.tx);

    glClearColor(1.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(kugl.shader.name);

    matrix4array_t projection;
    projection.matrix = m4_defaultortho(0.0, bitmap->w, bitmap->h, 0, 0.0, 1.0);

    glUniformMatrix4fv(kugl.shader.uni_loc[0], 1, 0, projection.array);
    glViewport(0, 0, bitmap->w, bitmap->h);

    glDrawArrays(GL_TRIANGLES, 0, kugl.floatbuffer->pos);
    glBindVertexArray(0);
}

/* render one quad */

void ku_gl_render_quad(ku_bitmap_t* bitmap, uint32_t index, bmr_t mask)
{
    /* glBindFramebuffer(GL_FRAMEBUFFER, tex_frame.fb); */
    glBindVertexArray(kugl.buffer.vao);

    glActiveTexture(GL_TEXTURE0 + kugl.texture.index);
    glBindTexture(GL_TEXTURE_2D, kugl.texture.tx);

    glUseProgram(kugl.shader.name);

    /* draw masked region only */

    matrix4array_t projection;
    projection.matrix = m4_defaultortho(mask.x, mask.z, mask.w, mask.y, 0.0, 1.0);

    glUniformMatrix4fv(kugl.shader.uni_loc[0], 1, 0, projection.array);

    /* viewport origo is in the left bottom corner */

    glViewport(mask.x, bitmap->h - mask.w, mask.z - mask.x, mask.w - mask.y);

    glDrawArrays(GL_TRIANGLES, index * 6, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* clear one rect */

void ku_gl_clear_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, bitmap->h - (x + w), w, h);
    /* glScissor(100, 100, 500, 500); */

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

/* save framebuffer to bitmap */

void ku_gl_save_framebuffer(ku_bitmap_t* bitmap)
{
    glReadPixels(0, 0, bitmap->w, bitmap->h, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->data);
}

#endif
