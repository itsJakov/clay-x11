#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_icccm.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#define CLAY_IMPLEMENTATION
#include "../../clay.h"

const Clay_Color COLOR_LIGHT = (Clay_Color) {224, 215, 210, 255};
const Clay_Color COLOR_RED = (Clay_Color) {168, 66, 28, 255};
const Clay_Color COLOR_ORANGE = (Clay_Color) {225, 138, 50, 255};

#define MIN(a, b) \
    ({ __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b; })

const char* Clay_StringSliceIntoCString(Clay_StringSlice slice) {
    static char* tempBuffer = 0;
    static size_t bufferLen = 1024;
    if (tempBuffer == 0) {
        tempBuffer = malloc(MIN(bufferLen, slice.length));
    } else if (bufferLen < slice.length) {
        free(tempBuffer);
        tempBuffer = malloc(slice.length);
        bufferLen = slice.length;
    }

    strncpy(tempBuffer, slice.chars, slice.length);
    tempBuffer[slice.length] = '\0';
    return tempBuffer;
}

Clay_Dimensions measureTextFunction(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    cairo_t *cr = userData;
    const char* str = Clay_StringSliceIntoCString(text);

    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, config->fontSize);
    cairo_text_extents_t extents;
    cairo_text_extents(cr, str, &extents);

    return (Clay_Dimensions){ extents.width, extents.height };
}

void clay_setup() {
    uint32_t minSize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(minSize, malloc(minSize));
    Clay_Initialize(arena, (Clay_Dimensions){ 1280, 720 }, (Clay_ErrorHandler){ 0, 0 });

    Clay_SetDebugModeEnabled(true);
}

// Layout config is just a struct that can be declared statically, or inline
Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration) {
    .layout = {
        .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) }
    },
    .backgroundColor = COLOR_ORANGE
};

// Re-useable components are just normal functions
void SidebarItemComponent() {
    CLAY_AUTO_ID(sidebarItemConfig) {
        // children go here...
    }
}

Clay_RenderCommandArray do_layout() {
    Clay_BeginLayout();

    // An example of laying out a UI with a fixed width sidebar and flexible width main content
    CLAY(CLAY_ID("OuterContainer"), { .layout = { .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16 }, .backgroundColor = {250,250,255,255} }) {
        CLAY(CLAY_ID("SideBar"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16 },
            .backgroundColor = COLOR_LIGHT
        }) {
            CLAY(CLAY_ID("ProfilePictureOuter"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = COLOR_RED }) {
                CLAY(CLAY_ID("ProfilePicture"), {.layout = { .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }} }) {}
                CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
            }

            for (int i = 0; i < 5; i++) {
                SidebarItemComponent();
            }

            CLAY(CLAY_ID("MainContent"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = COLOR_LIGHT }) {}
        }
    }

    return Clay_EndLayout();
}

Clay_RenderCommandArray do_layout2() {
    Clay_BeginLayout();

    CLAY(CLAY_ID("OuterContainer"), { .layout = { .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16) }, .backgroundColor = {250,250,255,255} }) {
        CLAY(CLAY_ID("SideBar"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 16 },
            .backgroundColor = COLOR_RED
        }) {
            CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
        }
    }

    return Clay_EndLayout();
}

void draw(xcb_connection_t *conn, xcb_window_t window, cairo_surface_t *surface, cairo_t *cr) {
    /*xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry_unchecked(conn, window);
    xcb_get_geometry_reply_t *geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);
    Clay_SetLayoutDimensions((Clay_Dimensions) { geometry_reply->width, geometry_reply->height});
    cairo_xcb_surface_set_size(surface, geometry_reply->width, geometry_reply->height);
    free(geometry_reply);

    xcb_query_pointer_reply_t * pointer_info = xcb_query_pointer_reply(conn, xcb_query_pointer_unchecked(conn, window), NULL);
    Clay_SetPointerState((Clay_Vector2){ pointer_info->win_x, pointer_info->win_y}, false);
    free(pointer_info);*/

    cairo_push_group(cr);

    Clay_RenderCommandArray renderCommands = do_layout2();
    for (int i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommand *cmd = &renderCommands.internalArray[i];
        switch (cmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_BoundingBox *box = &cmd->boundingBox;
                Clay_Color *color = &cmd->renderData.rectangle.backgroundColor;

                cairo_set_source_rgba(cr, color->r / 255.0, color->g / 255.0, color->b / 255.0, color->a / 255.0);
                cairo_rectangle(cr, box->x, box->y, box->width, box->height);
                cairo_fill(cr);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_BoundingBox *box = &cmd->boundingBox;
                Clay_TextRenderData *text = &cmd->renderData.text;

                cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, text->fontSize);
                cairo_set_source_rgb(cr, text->textColor.r / 255.0, text->textColor.r / 255.0, text->textColor.r / 255.0);

                const char* cstr = Clay_StringSliceIntoCString(text->stringContents);
                cairo_text_extents_t extents;
                cairo_text_extents(cr, cstr, &extents);

                cairo_move_to(cr, box->x - extents.x_bearing, box->y - extents.y_bearing);
                cairo_show_text(cr, cstr);
                break;
            }
            default: break;
        }
    }

    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, 1);

    cairo_surface_flush(surface);
    xcb_flush(conn);
}

int main() {
    clay_setup();

    xcb_connection_t *conn = xcb_connect(NULL, NULL);

    const struct xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;



    const xcb_window_t window = xcb_generate_id(conn);
    xcb_create_window_aux(conn,
        screen->root_depth,
        window,
        screen->root,
        10, 10, 1280, 720,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_EVENT_MASK,
        &(xcb_create_window_value_list_t){
            .event_mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION
        }
        );
    xcb_map_window(conn, window);
    const char* WINDOW_TITLE = "Hello, World";
    xcb_icccm_set_wm_name(conn, window, XCB_ATOM_STRING, 8, strlen(WINDOW_TITLE), WINDOW_TITLE);

    cairo_surface_t *surface = cairo_xcb_surface_create(conn, window, xcb_aux_find_visual_by_id(screen, screen->root_visual), 1280, 720);
    cairo_t *cr = cairo_create(surface);
    Clay_SetMeasureTextFunction(&measureTextFunction, cr);

    xcb_flush(conn);

    xcb_generic_event_t *event;



    while (true) {
        event = xcb_poll_for_event(conn);
        draw(conn, window, surface, cr);

        /*const char *str = "Hello, World";

        cairo_push_group(cr);

        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 120);

        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_paint(cr);
        cairo_surface_flush(surface);

        cairo_text_extents_t extents;
        cairo_text_extents(cr, str, &extents);

        cairo_set_source_rgb(cr, 0, 1, 1);
        //cairo_rectangle(cr, 10, 10, extents.x_advance, extents.height);
        cairo_rectangle(cr, 10, 10, extents.width, extents.height);
        cairo_fill(cr);

        cairo_font_extents_t fontExtents;
        cairo_font_extents(cr, &fontExtents);

        cairo_set_source_rgb(cr, 0, 0, 0);
        printf("x_bearing: %f\n", extents.x_bearing);
        cairo_move_to(cr, 10, fontExtents.ascent + (extents.height - fontExtents.ascent - fontExtents.descent) / 2);
        //cairo_move_to(cr, 10 - extents.x_bearing, 10 - extents.y_bearing);
        cairo_show_text(cr, str);

        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1);

        cairo_surface_flush(surface);
        xcb_flush(conn);*/

        usleep(500);
        //usleep(1000*1000/120);
    }
    /*while ((event = xcb_wait_for_event(conn))) {
        switch (XCB_EVENT_RESPONSE_TYPE(event)) {
            case XCB_RESIZE_REQUEST: {
                printf("resizing!\n");
                //break;
            }
            case XCB_MOTION_NOTIFY:
            case XCB_EXPOSE: {
                printf("expose!\n");
                draw(conn, window, surface, cr);

                break;
            }
            default:
                fprintf(stderr, "Unknown error received!\n");
        }
    }*/

    xcb_disconnect(conn);
    return 0;
}
