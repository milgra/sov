#ifndef ku_gen_html_h
#define ku_gen_html_h

#include "mt_vector.c"

void ku_gen_html_parse(char* htmlpath, mt_vector_t* views);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_html.c"
#include "ku_view.c"
#include "mt_log.c"
#include "mt_string_ext.c"

void ku_gen_html_parse(char* htmlpath, mt_vector_t* views)
{
    char* html = mt_string_new_file(htmlpath); // REL 0

    if (html != NULL)
    {
	tag_t* tags = ku_html_new(html); // REL 1
	tag_t* head = tags;

	while ((*tags).len > 0)
	{
	    tag_t t = *tags;
	    if (t.id.len > 0)
	    {
		// extract id
		char* id = CAL(sizeof(char) * t.id.len + 1, NULL, mt_string_describe); // REL 0
		memcpy(id, html + t.id.pos + 1, t.id.len);
		ku_view_t* view = ku_view_new(id, (ku_rect_t){0}); // REL 1

		if (t.level > 0)
		{
		    // add to parent
		    ku_view_t* parent = views->data[t.parent];
		    ku_view_add_subview(parent, view);
		}

		if (t.class.len > 0)
		{
		    // store css classes
		    char* class = CAL(sizeof(char) * t.class.len + 1, NULL, mt_string_describe); // REL 0
		    memcpy(class, html + t.class.pos + 1, t.class.len);
		    ku_view_set_class(view, class);
		    REL(class);
		}

		if (t.type.len > 0)
		{
		    // store html stype
		    char* type = CAL(sizeof(char) * t.type.len + 1, NULL, mt_string_describe); // REL 2
		    memcpy(type, html + t.type.pos + 1, t.type.len);
		    ku_view_set_type(view, type);
		    REL(type); // REL 2
		}

		if (t.text.len > 0)
		{
		    // store html stype
		    char* text = CAL(sizeof(char) * t.text.len + 1, NULL, mt_string_describe); // REL 2
		    memcpy(text, html + t.text.pos + 1, t.text.len);
		    ku_view_set_text(view, text);
		    REL(text); // REL 2
		}

		if (t.script.len > 0)
		{
		    // store html stype
		    char* script = CAL(sizeof(char) * t.script.len + 1, NULL, mt_string_describe); // REL 2
		    memcpy(script, html + t.script.pos + 1, t.script.len);
		    ku_view_set_script(view, script);
		    REL(script); // REL 2
		}

		VADD(views, view);

		REL(id);   // REL 0
		REL(view); // REL 1
	    }
	    else
	    {
		static int divcnt = 0;
		char*      divid  = mt_string_new_format(10, "div%i", divcnt++);
		// idless view, probably </div>
		ku_view_t* view = ku_view_new(divid, (ku_rect_t){0});
		VADD(views, view);
		REL(view);
		REL(divid);
	    }
	    tags += 1;
	}

	// cleanup

	REL(head); // REL 1
	REL(html); // REL 0
    }
    else mt_log_error("No HTML description");
}

#endif
