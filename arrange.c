#include "arrange.h"

#include <string.h>

#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/util/log.h>

#include "layer_shell.h"

#define LENGTH(X) (sizeof X / sizeof X[0])

static void
apply_exclusive(struct wlr_box *usable_area, uint32_t anchor, int32_t exclusive, int32_t margin_top,
		int32_t margin_right, int32_t margin_bottom, int32_t margin_left)
{
	if (exclusive <= 0) {
		return;
	}
	struct {
		uint32_t singular_anchor;
		uint32_t anchor_triplet;
		int *positive_axis;
		int *negative_axis;
		int margin;
	} edges[] = {
		// Top
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			.positive_axis = &usable_area->y,
			.negative_axis = &usable_area->height,
			.margin = margin_top,
		},
		// Bottom
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = NULL,
			.negative_axis = &usable_area->height,
			.margin = margin_bottom,
		},
		// Left
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = &usable_area->x,
			.negative_axis = &usable_area->width,
			.margin = margin_left,
		},
		// Right
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = NULL,
			.negative_axis = &usable_area->width,
			.margin = margin_right,
		},
	};
	for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
		if ((anchor == edges[i].singular_anchor || anchor == edges[i].anchor_triplet) &&
		    exclusive + edges[i].margin > 0) {
			if (edges[i].positive_axis) {
				*edges[i].positive_axis += exclusive + edges[i].margin;
			}
			if (edges[i].negative_axis) {
				*edges[i].negative_axis -= exclusive + edges[i].margin;
			}
			break;
		}
	}
}

static void
arrange_layer(struct cg_output *output, struct wl_list *list, struct wlr_box *usable_area, bool exclusive)
{
	struct cg_layer_shell *layer_shell;
	struct wlr_box full_area = {0};
	wlr_output_effective_resolution(output->wlr_output, &full_area.width, &full_area.height);

	wl_list_for_each (layer_shell, list, link) {
		struct wlr_layer_surface_v1 *wlr_layer_surface = layer_shell->wlr_layer_surface;
		struct wlr_layer_surface_v1_state *state = &wlr_layer_surface->current;
		struct wlr_box bounds;
		struct wlr_box box = {.width = state->desired_width, .height = state->desired_height};
		const uint32_t both_horiz = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
		const uint32_t both_vert = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;

		if (exclusive != (state->exclusive_zone > 0))
			continue;

		bounds = state->exclusive_zone == -1 ? full_area : *usable_area;

		// Horizontal axis
		if ((state->anchor & both_horiz) && box.width == 0) {
			box.x = bounds.x;
			box.width = bounds.width;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
			box.x = bounds.x;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
			box.x = bounds.x + (bounds.width - box.width);
		} else {
			box.x = bounds.x + ((bounds.width / 2) - (box.width / 2));
		}
		// Vertical axis
		if ((state->anchor & both_vert) && box.height == 0) {
			box.y = bounds.y;
			box.height = bounds.height;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
			box.y = bounds.y;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
			box.y = bounds.y + (bounds.height - box.height);
		} else {
			box.y = bounds.y + ((bounds.height / 2) - (box.height / 2));
		}
		// Margin
		if ((state->anchor & both_horiz) == both_horiz) {
			box.x += state->margin.left;
			box.width -= state->margin.left + state->margin.right;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
			box.x += state->margin.left;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
			box.x -= state->margin.right;
		}
		if ((state->anchor & both_vert) == both_vert) {
			box.y += state->margin.top;
			box.height -= state->margin.top + state->margin.bottom;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
			box.y += state->margin.top;
		} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
			box.y -= state->margin.bottom;
		}
		if (box.width < 0 || box.height < 0) {
			wlr_layer_surface_v1_close(wlr_layer_surface);
			continue;
		}
		layer_shell->geo = box;

		if (state->exclusive_zone > 0)
			apply_exclusive(usable_area, state->anchor, state->exclusive_zone, state->margin.top,
					state->margin.right, state->margin.bottom, state->margin.left);

		wlr_log(WLR_DEBUG, "Layer configured: %dx%d, %d", box.width, box.height, state->exclusive_zone);
		wlr_layer_surface_v1_configure(wlr_layer_surface, box.width, box.height);
	}
}

void
arrange_layers(struct cg_output *output, struct wlr_box *usable_area)
{
	// Arrange exclusive surfaces from top->bottom
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], usable_area, 1);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], usable_area, 1);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], usable_area, 1);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], usable_area, 1);

	// Arrange non-exlusive surfaces from top->bottom
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], usable_area, 0);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], usable_area, 0);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], usable_area, 0);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], usable_area, 0);

	// Find topmost keyboard interactive layer, if such a layer exists
	// TODO seat not available
	// struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);
	// size_t nlayers = LENGTH(layers_above_shell);
	// struct cg_layer_shell *layer_shell;
	// 	uint32_t layers_above_shell[] = {
	// 	ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
	// 	ZWLR_LAYER_SHELL_V1_LAYER_TOP,
	// };
	// for (size_t i = 0; i < nlayers; ++i) {
	// 	wl_list_for_each_reverse (layer_shell, &m->layers[layers_above_shell[i]], link) {
	// 		if (layer_shell->wlr_layer_surface->current.keyboard_interactive &&
	// 		    layer_shell->wlr_layer_surface->mapped) {
	// 			// Deactivate the focused client.
	// 			focusclient(NULL, 0);

	// 			wlr_seat_keyboard_notify_enter(seat, layer_shell->wlr_layer_surface->surface,
	// 						       kb->keycodes, kb->num_keycodes, &kb->modifiers);
	// 			return;
	// 		}
	// 	}
	// }
}

void
arrange_output(struct cg_output *output)
{
	struct wlr_box usable_area = {0};
	wlr_output_effective_resolution(output->wlr_output, &usable_area.width, &usable_area.height);
	arrange_layers(output, &usable_area);

	if (memcmp(&usable_area, &output->usable_area, sizeof(struct wlr_box))) {
		wlr_log(WLR_DEBUG, "Usable area changed. Old: %dx%d, New: %dx%d. Repositioning",
			output->usable_area.width, output->usable_area.height, usable_area.width, usable_area.height);
		memcpy(&output->usable_area, &usable_area, sizeof(struct wlr_box));
	}
}

void
arrange_server(struct cg_server *server)
{
	struct cg_output *output;
	wl_list_for_each (output, &server->outputs, link) {
		arrange_output(output);
	}
	struct cg_view *view;
	wl_list_for_each (view, &server->views, link) {
		view_position(view);
	}
}
